/*
    Copyright (C) 1999-2002 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id$
*/

#include <algorithm>
#include <string>
#include <cmath>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <glibmm/thread.h>

#include <pbd/error.h>
#include <pbd/pthread_utils.h>

#include <ardour/configuration.h>
#include <ardour/audioengine.h>
#include <ardour/session.h>
#include <ardour/audio_diskstream.h>
#include <ardour/crossfade.h>
#include <ardour/timestamps.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

static float _read_data_rate;
static float _write_data_rate;

/* XXX put this in the right place */

static inline uint32_t next_power_of_two (uint32_t n)
{
	--n;
	n |= n >> 16;
	n |= n >> 8;
	n |= n >> 4;
	n |= n >> 2;
	n |= n >> 1;
	++n;
	return n;
}

/*---------------------------------------------------------------------------
 BUTLER THREAD 
 ---------------------------------------------------------------------------*/

int
Session::start_butler_thread ()
{
	/* size is in Samples, not bytes */

	dstream_buffer_size = (uint32_t) floor (Config->get_track_buffer_seconds() * (float) frame_rate());

	Crossfade::set_buffer_size (dstream_buffer_size);

	butler_should_run = false;

	if (pipe (butler_request_pipe)) {
		error << string_compose(_("Cannot create transport request signal pipe (%1)"), strerror (errno)) << endmsg;
		return -1;
	}

	if (fcntl (butler_request_pipe[0], F_SETFL, O_NONBLOCK)) {
		error << string_compose(_("UI: cannot set O_NONBLOCK on butler request pipe (%1)"), strerror (errno)) << endmsg;
		return -1;
	}

	if (fcntl (butler_request_pipe[1], F_SETFL, O_NONBLOCK)) {
		error << string_compose(_("UI: cannot set O_NONBLOCK on butler request pipe (%1)"), strerror (errno)) << endmsg;
		return -1;
	}

	if (pthread_create_and_store ("disk butler", &butler_thread, 0, _butler_thread_work, this)) {
		error << _("Session: could not create butler thread") << endmsg;
		return -1;
	}

	// pthread_detach (butler_thread);

	return 0;
}

void
Session::terminate_butler_thread ()
{
	void* status;
	char c = ButlerRequest::Quit;
	::write (butler_request_pipe[1], &c, 1);
	pthread_join (butler_thread, &status);
}

void
Session::schedule_butler_transport_work ()
{
	g_atomic_int_inc (&butler_should_do_transport_work);
	summon_butler ();
}

void
Session::schedule_curve_reallocation ()
{
	post_transport_work = PostTransportWork (post_transport_work | PostTransportCurveRealloc);
	schedule_butler_transport_work ();
}

void
Session::summon_butler ()
{
	char c = ButlerRequest::Run;
	::write (butler_request_pipe[1], &c, 1);
}

void
Session::stop_butler ()
{
	Glib::Mutex::Lock lm (butler_request_lock);
	char c = ButlerRequest::Pause;
	::write (butler_request_pipe[1], &c, 1);
	butler_paused.wait(butler_request_lock);
}

void
Session::wait_till_butler_finished ()
{
	Glib::Mutex::Lock lm (butler_request_lock);
	char c = ButlerRequest::Wake;
	::write (butler_request_pipe[1], &c, 1);
	butler_paused.wait(butler_request_lock);
}

void *
Session::_butler_thread_work (void* arg)
{
	PBD::ThreadCreated (pthread_self(), X_("Butler"));
	return ((Session *) arg)->butler_thread_work ();
	return 0;
}

#define transport_work_requested() g_atomic_int_get(&butler_should_do_transport_work)

void *
Session::butler_thread_work ()
{
	uint32_t err = 0;
	int32_t bytes;
	bool compute_io;
	struct timeval begin, end;
	struct pollfd pfd[1];
	bool disk_work_outstanding = false;
	DiskstreamList::iterator i;

	butler_mixdown_buffer = new Sample[AudioDiskstream::disk_io_frames()];
	butler_gain_buffer = new gain_t[AudioDiskstream::disk_io_frames()];
	// this buffer is used for temp conversion purposes in filesources
	char * conv_buffer = conversion_buffer(ButlerContext);
	
	while (true) {
		pfd[0].fd = butler_request_pipe[0];
		pfd[0].events = POLLIN|POLLERR|POLLHUP;
		
		if (poll (pfd, 1, (disk_work_outstanding ? 0 : -1)) < 0) {
			
			if (errno == EINTR) {
				continue;
			}
			
			error << string_compose (_("poll on butler request pipe failed (%1)"),
					  strerror (errno))
			      << endmsg;
			break;
		}

		if (pfd[0].revents & ~POLLIN) {
			error << _("Error on butler thread request pipe") << endmsg;
			break;
		}
		
		if (pfd[0].revents & POLLIN) {

			char req;
			
			/* empty the pipe of all current requests */
			
			while (1) {
				size_t nread = ::read (butler_request_pipe[0], &req, sizeof (req));
				if (nread == 1) {
					
					switch ((ButlerRequest::Type) req) {
						
					case ButlerRequest::Wake:
						break;

					case ButlerRequest::Run:
						butler_should_run = true;
						break;
						
					case ButlerRequest::Pause:
						butler_should_run = false;
						break;
						
					case ButlerRequest::Quit:
						pthread_exit_pbd (0);
						/*NOTREACHED*/
						break;
						
					default:
						break;
					}
					
				} else if (nread == 0) {
					break;
				} else if (errno == EAGAIN) {
					break;
				} else {
					fatal << _("Error reading from butler request pipe") << endmsg;
					/*NOTREACHED*/
				}
			}
		}

		//for (i = diskstreams.begin(); i != diskstreams.end(); ++i) {
			// cerr << "BEFORE " << (*i)->name() << ": pb = " << (*i)->playback_buffer_load() << " cp = " << (*i)->capture_buffer_load() << endl;
		//}

		if (transport_work_requested()) {
			butler_transport_work ();
		}

		disk_work_outstanding = false;
		bytes = 0;
		compute_io = true;

		gettimeofday (&begin, 0);

		Glib::RWLock::ReaderLock dsm (diskstream_lock);
		
		for (i = diskstreams.begin(); !transport_work_requested() && butler_should_run && i != diskstreams.end(); ++i) {
			// cerr << "rah fondr " << (*i)->io()->name () << endl;
			AudioDiskstream* ads = dynamic_cast<AudioDiskstream*>(*i);
			if (!ads) continue; // FIXME

			switch (ads->do_refill (butler_mixdown_buffer, butler_gain_buffer, conv_buffer)) {
			case 0:
				bytes += ads->read_data_count();
				break;
			case 1:
				bytes += ads->read_data_count();
				disk_work_outstanding = true;
				break;
				
			default:
				compute_io = false;
				error << string_compose(_("Butler read ahead failure on dstream %1"), (*i)->name()) << endmsg;
				break;
			}

		}

		if (i != diskstreams.end()) {
			/* we didn't get to all the streams */
			disk_work_outstanding = true;
		}
		
		if (!err && transport_work_requested()) {
			continue;
		}

		if (compute_io) {
			gettimeofday (&end, 0);
			
			double b = begin.tv_sec  + (begin.tv_usec/1000000.0);
			double e = end.tv_sec + (end.tv_usec / 1000000.0);
			
			_read_data_rate = bytes / (e - b);
		}

		bytes = 0;
		compute_io = true;
		gettimeofday (&begin, 0);

		for (i = diskstreams.begin(); !transport_work_requested() && butler_should_run && i != diskstreams.end(); ++i) {
			// cerr << "write behind for " << (*i)->name () << endl;
			
			switch ((*i)->do_flush (conv_buffer)) {
			case 0:
				bytes += (*i)->write_data_count();
				break;
			case 1:
				bytes += (*i)->write_data_count();
				disk_work_outstanding = true;
				break;
				
			default:
				err++;
				compute_io = false;
				error << string_compose(_("Butler write-behind failure on dstream %1"), (*i)->name()) << endmsg;
				/* don't break - try to flush all streams in case they 
				   are split across disks.
				*/
			}
		}

		if (err && actively_recording()) {
			/* stop the transport and try to catch as much possible
			   captured state as we can.
			*/
			request_stop ();
		}

		if (i != diskstreams.end()) {
			/* we didn't get to all the streams */
			disk_work_outstanding = true;
		}
		
		if (!err && transport_work_requested()) {
			continue;
		}

		if (compute_io) {
			gettimeofday (&end, 0);
			
			double b = begin.tv_sec  + (begin.tv_usec/1000000.0);
			double e = end.tv_sec + (end.tv_usec / 1000000.0);
			
			_write_data_rate = bytes / (e - b);
		}
		
		if (!disk_work_outstanding) {
			refresh_disk_space ();
		}


		{
			Glib::Mutex::Lock lm (butler_request_lock);

			if (butler_should_run && (disk_work_outstanding || transport_work_requested())) {
//				for (DiskstreamList::iterator i = diskstreams.begin(); i != diskstreams.end(); ++i) {
//					cerr << "AFTER " << (*i)->name() << ": pb = " << (*i)->playback_buffer_load() << " cp = " << (*i)->capture_buffer_load() << endl;
//				}

				continue;
			}

			butler_paused.signal();
		}
	}

	pthread_exit_pbd (0);
	/*NOTREACHED*/
	return (0);
}


void
Session::request_overwrite_buffer (Diskstream* stream)
{
	Event *ev = new Event (Event::Overwrite, Event::Add, Event::Immediate, 0, 0, 0.0);
	ev->set_ptr (stream);
	queue_event (ev);
}

/** Process thread. */
void
Session::overwrite_some_buffers (Diskstream* ds)
{
	if (actively_recording()) {
		return;
	}

	if (ds) {

		ds->set_pending_overwrite (true);

	} else {

		Glib::RWLock::ReaderLock dm (diskstream_lock);
		for (DiskstreamList::iterator i = diskstreams.begin(); i != diskstreams.end(); ++i) {
			(*i)->set_pending_overwrite (true);
		}
	}

	post_transport_work = PostTransportWork (post_transport_work | PostTransportOverWrite);
	schedule_butler_transport_work ();
}

float
Session::read_data_rate () const
{
	/* disk i/o in excess of 10000MB/sec indicate the buffer cache
	   in action. ignore it.
	*/
	return _read_data_rate > 10485760000.0f ? 0.0f : _read_data_rate;
}

float
Session::write_data_rate () const
{
	/* disk i/o in excess of 10000MB/sec indicate the buffer cache
	   in action. ignore it.
	*/
	return _write_data_rate > 10485760000.0f ? 0.0f : _write_data_rate;
}

uint32_t
Session::playback_load ()
{
	return (uint32_t) g_atomic_int_get (&_playback_load);
}

uint32_t
Session::capture_load ()
{
	return (uint32_t) g_atomic_int_get (&_capture_load);
}

uint32_t
Session::playback_load_min ()
{
	return (uint32_t) g_atomic_int_get (&_playback_load_min);
}

uint32_t
Session::capture_load_min ()
{
	return (uint32_t) g_atomic_int_get (&_capture_load_min);
}

void
Session::reset_capture_load_min ()
{
	g_atomic_int_set (&_capture_load_min, 100);
}


void
Session::reset_playback_load_min ()
{
	g_atomic_int_set (&_playback_load_min, 100);
}
