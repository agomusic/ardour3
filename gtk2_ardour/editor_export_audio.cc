/*
    Copyright (C) 2001 Paul Davis 

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

*/

/* Note: public Editor methods are documented in public_editor.h */

#include <unistd.h>
#include <climits>

#include <gtkmm/messagedialog.h>

#include "export_main_dialog.h"
#include "editor.h"
#include "public_editor.h"
#include "selection.h"
#include "time_axis_view.h"
#include "audio_time_axis.h"
#include "audio_region_view.h"

#include <pbd/pthread_utils.h>
#include <ardour/types.h>
#include <ardour/audio_track.h>
#include <ardour/audiofilesource.h>
#include <ardour/audio_diskstream.h>
#include <ardour/audioregion.h>
#include <ardour/audioplaylist.h>
#include <ardour/chan_count.h>
#include <ardour/session_directory.h>
#include <ardour/source_factory.h>
#include <ardour/audiofilesource.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;

void
Editor::export_audio ()
{
	ExportMainDialog dialog (*this);
	dialog.set_session (session);
	dialog.run();
}

void
Editor::export_selection ()
{
	ExportMainDialog dialog (*this);
	dialog.set_session (session);
	dialog.select_timespan (X_("selection"));
	dialog.run();
}

void
Editor::export_range ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (marker_menu_item->get_data ("marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;

	if (((l = find_location_from_marker (marker, is_start)) != 0) && (l->end() > l->start())) {
		ExportMainDialog dialog (*this);
		dialog.set_session (session);
		dialog.select_timespan (l->id().to_s());
		dialog.run();
	}
}

/** Export the first selected region */
void
Editor::export_region ()
{
// 	if (selection->regions.empty()) {
// 		return;
// 	}
// 
// 	boost::shared_ptr<Region> r = selection->regions.front()->region();
// 	
// 	ExportDialog* dialog = new ExportRegionDialog (*this, r);
// 		
// 	dialog->connect_to_session (session);
// 	dialog->set_range (clicked_regionview->region()->first_frame(), clicked_regionview->region()->last_frame());
// 	dialog->start_export();
}

int
Editor::write_region_selection (RegionSelection& regions)
{
	for (RegionSelection::iterator i = regions.begin(); i != regions.end(); ++i) {
		// FIXME
		AudioRegionView* arv = dynamic_cast<AudioRegionView*>(*i);
		if (arv)
			if (write_region ("", arv->audio_region()) == false)
				return -1;
	}

	return 0;
}

void
Editor::bounce_region_selection ()
{
	for (RegionSelection::iterator i = selection->regions.begin(); i != selection->regions.end(); ++i) {
		
		boost::shared_ptr<Region> region ((*i)->region());
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(&(*i)->get_time_axis_view());
		Track* track = dynamic_cast<Track*>(rtv->route().get());

		InterThreadInfo itt;

		itt.done = false;
		itt.cancel = false;
		itt.progress = 0.0f;

		boost::shared_ptr<Region> r = track->bounce_range (region->position(), region->position() + region->length(), itt);
		cerr << "Result of bounce of "
		     << region->name() << " len = " << region->length()
		     << " was "
		     << r->name() << " len = " << r->length()
		     << endl;
	}
}

bool
Editor::write_region (string path, boost::shared_ptr<AudioRegion> region)
{
	boost::shared_ptr<AudioFileSource> fs;
	const nframes64_t chunk_size = 4096;
	nframes64_t to_read;
	Sample buf[chunk_size];
	gain_t gain_buffer[chunk_size];
	nframes64_t pos;
	char s[PATH_MAX+1];
	uint32_t cnt;
	vector<boost::shared_ptr<AudioFileSource> > sources;
	uint32_t nchans;

	const string sound_directory = session->session_directory().sound_path().to_string();

	nchans = region->n_channels();
	
	/* don't do duplicate of the entire source if that's what is going on here */

	if (region->start() == 0 && region->length() == region->source()->length()) {
		/* XXX should link(2) to create a new inode with "path" */
		return true;
	}

	if (path.length() == 0) {

		for (uint32_t n=0; n < nchans; ++n) {
			
			for (cnt = 0; cnt < 999999; ++cnt) {
				if (nchans == 1) {
					snprintf (s, sizeof(s), "%s/%s_%" PRIu32 ".wav", sound_directory.c_str(),
						  legalize_for_path(region->name()).c_str(), cnt);
				}
				else {
					snprintf (s, sizeof(s), "%s/%s_%" PRIu32 "-%" PRId32 ".wav", sound_directory.c_str(),
						  legalize_for_path(region->name()).c_str(), cnt, n);
				}

				path = s;
				
				if (::access (path.c_str(), F_OK) != 0) {
					break;
				}
			}
			
			if (cnt == 999999) {
				error << "" << endmsg;
				goto error_out;
			}
			
		
			
			try {
				fs = boost::dynamic_pointer_cast<AudioFileSource> (SourceFactory::createWritable (DataType::AUDIO, *session, path, false, session->frame_rate()));
			}
			
			catch (failed_constructor& err) {
				goto error_out;
			}

			sources.push_back (fs);
		}
	}
	else {
		/* TODO: make filesources based on passed path */

	}
	
	to_read = region->length();
	pos = region->position();

	while (to_read) {
		nframes64_t this_time;

		this_time = min (to_read, chunk_size);

		for (vector<boost::shared_ptr<AudioFileSource> >::iterator src=sources.begin(); src != sources.end(); ++src) {
			
			fs = (*src);

			if (region->read_at (buf, buf, gain_buffer, pos, this_time) != this_time) {
				break;
			}
			
			if (fs->write (buf, this_time) != this_time) {
				error << "" << endmsg;
				goto error_out;
			}
		}

		to_read -= this_time;
		pos += this_time;
	}

	time_t tnow;
	struct tm* now;
	time (&tnow);
	now = localtime (&tnow);
	
	for (vector<boost::shared_ptr<AudioFileSource> >::iterator src = sources.begin(); src != sources.end(); ++src) {
		(*src)->update_header (0, *now, tnow);
		(*src)->mark_immutable ();
	}

	return true;

error_out:

	for (vector<boost::shared_ptr<AudioFileSource> >::iterator i = sources.begin(); i != sources.end(); ++i) {
		(*i)->mark_for_remove ();
	}

	return 0;
}

int
Editor::write_audio_selection (TimeSelection& ts)
{
	int ret = 0;

	if (selection->tracks.empty()) {
		return 0;
	}

	for (TrackSelection::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {

		AudioTimeAxisView* atv;

		if ((atv = dynamic_cast<AudioTimeAxisView*>(*i)) == 0) {
			continue;
		}

		if (atv->is_audio_track()) {

			boost::shared_ptr<AudioPlaylist> playlist = boost::dynamic_pointer_cast<AudioPlaylist>(atv->get_diskstream()->playlist());
			
			if (playlist && write_audio_range (*playlist, atv->get_diskstream()->n_channels(), ts) == 0) {
				ret = -1;
				break;
			}
		}
	}

	return ret;
}

bool
Editor::write_audio_range (AudioPlaylist& playlist, const ChanCount& count, list<AudioRange>& range)
{
	boost::shared_ptr<AudioFileSource> fs;
	const nframes64_t chunk_size = 4096;
	nframes64_t nframes;
	Sample buf[chunk_size];
	gain_t gain_buffer[chunk_size];
	nframes64_t pos;
	char s[PATH_MAX+1];
	uint32_t cnt;
	string path;
	vector<boost::shared_ptr<AudioFileSource> > sources;

	const string sound_directory = session->session_directory().sound_path().to_string();

	uint32_t channels = count.n_audio();

	for (uint32_t n=0; n < channels; ++n) {
		
		for (cnt = 0; cnt < 999999; ++cnt) {
			if (channels == 1) {
				snprintf (s, sizeof(s), "%s/%s_%" PRIu32 ".wav", sound_directory.c_str(),
					  legalize_for_path(playlist.name()).c_str(), cnt);
			}
			else {
				snprintf (s, sizeof(s), "%s/%s_%" PRIu32 "-%" PRId32 ".wav", sound_directory.c_str(),
					  legalize_for_path(playlist.name()).c_str(), cnt, n);
			}
			
			if (::access (s, F_OK) != 0) {
				break;
			}
		}
		
		if (cnt == 999999) {
			error << "" << endmsg;
			goto error_out;
		}

		path = s;
		
		try {
			fs = boost::dynamic_pointer_cast<AudioFileSource> (SourceFactory::createWritable (DataType::AUDIO, *session, path, false, session->frame_rate()));
		}
		
		catch (failed_constructor& err) {
			goto error_out;
		}
		
		sources.push_back (fs);

	}
	

	for (list<AudioRange>::iterator i = range.begin(); i != range.end();) {
	
		nframes = (*i).length();
		pos = (*i).start;
		
		while (nframes) {
			nframes64_t this_time;
			
			this_time = min (nframes, chunk_size);

			for (uint32_t n=0; n < channels; ++n) {

				fs = sources[n];
				
				if (playlist.read (buf, buf, gain_buffer, pos, this_time, n) != this_time) {
					break;
				}
				
				if (fs->write (buf, this_time) != this_time) {
					goto error_out;
				}
			}
			
			nframes -= this_time;
			pos += this_time;
		}
		
		list<AudioRange>::iterator tmp = i;
		++tmp;

		if (tmp != range.end()) {
			
			/* fill gaps with silence */
			
			nframes = (*tmp).start - (*i).end;

			while (nframes) {

				nframes64_t this_time = min (nframes, chunk_size);
				memset (buf, 0, sizeof (Sample) * this_time);

				for (uint32_t n=0; n < channels; ++n) {

					fs = sources[n];
					if (fs->write (buf, this_time) != this_time) {
						goto error_out;
					}
				}

				nframes -= this_time;
			}
		}

		i = tmp;
	}

	time_t tnow;
	struct tm* now;
	time (&tnow);
	now = localtime (&tnow);

	for (vector<boost::shared_ptr<AudioFileSource> >::iterator s = sources.begin(); s != sources.end(); ++s) {
		(*s)->update_header (0, *now, tnow);
		(*s)->mark_immutable ();
		// do we need to ref it again?
	}
	
	return true;

error_out:
	/* unref created files */

	for (vector<boost::shared_ptr<AudioFileSource> >::iterator i = sources.begin(); i != sources.end(); ++i) {
		(*i)->mark_for_remove ();
	}

	return false;
}

void
Editor::write_selection ()
{
	if (!selection->time.empty()) {
		write_audio_selection (selection->time);
	} else if (!selection->regions.empty()) {
		write_region_selection (selection->regions);
	}
}
