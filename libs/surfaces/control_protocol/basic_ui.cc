/*
    Copyright (C) 2006 Paul Davis 

    This program is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser
    General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id$
*/

#include <ardour/session.h>
#include <ardour/location.h>

#include "basic_ui.h"
#include "i18n.h"

using namespace ARDOUR;

BasicUI::BasicUI (Session& s)
	: session (&s)
{
}

BasicUI::BasicUI ()
	: session (0)
{
}

BasicUI::~BasicUI ()
{
	
}

void
BasicUI::loop_toggle () 
{
	if (session->get_auto_loop()) {
		session->request_auto_loop (false);
	} else {
		session->request_auto_loop (true);
		if (!session->transport_rolling()) {
			session->request_transport_speed (1.0);
		}
	}
}

void
BasicUI::goto_start ()
{
	session->goto_start ();
}

void
BasicUI::goto_end ()
{
	session->goto_end ();
}

void       
BasicUI::add_marker ()
{
	jack_nframes_t when = session->audible_frame();
	session->locations()->add (new Location (when, when, _("unnamed"), Location::IsMark));
}

void
BasicUI::rewind ()
{
	session->request_transport_speed (-2.0f);
}

void
BasicUI::ffwd ()
{
	session->request_transport_speed (2.0f);
}

void
BasicUI::transport_stop ()
{
	session->request_transport_speed (0.0);
}

void
BasicUI::transport_play (bool from_last_start)
{
	bool rolling = session->transport_rolling ();

	if (session->get_auto_loop()) {
		session->request_auto_loop (false);
	} 

	if (session->get_play_range ()) {
		session->request_play_range (false);
	}
	
	if (from_last_start && rolling) {
		session->request_locate (session->last_transport_start(), true);

	}

	session->request_transport_speed (1.0f);
}

void
BasicUI::rec_enable_toggle ()
{
	switch (session->record_status()) {
	case Session::Disabled:
		if (session->ntracks() == 0) {
			// string txt = _("Please create 1 or more track\nbefore trying to record.\nCheck the Session menu.");
			// MessageDialog msg (*editor, txt);
			// msg.run ();
			return;
		}
		session->maybe_enable_record ();
		break;
	case Session::Recording:
	case Session::Enabled:
		session->disable_record (true);
	}
}

void
BasicUI::save_state ()
{
	session->save_state ("");
}

void
BasicUI::prev_marker ()
{
	Location *location = session->locations()->first_location_before (session->transport_frame());
	
	if (location) {
		session->request_locate (location->start(), session->transport_rolling());
	} else {
		session->goto_start ();
	}
}

void
BasicUI::next_marker ()
{
	Location *location = session->locations()->first_location_after (session->transport_frame());

	if (location) {
		session->request_locate (location->start(), session->transport_rolling());
	} else {
		session->request_locate (session->current_end_frame());
	}
}

void
BasicUI::set_transport_speed (float speed)
{
	session->request_transport_speed (speed);
}

float
BasicUI::get_transport_speed ()
{
	return session->transport_speed ();
}

void
BasicUI::undo ()
{
	session->undo (1);
}

void
BasicUI::redo ()
{
	session->redo (1);
}

void
BasicUI::toggle_all_rec_enables ()
{
	if (session->get_record_enabled()) {
		session->record_disenable_all ();
	} else {
		session->record_enable_all ();
	}
}

void
BasicUI::toggle_punch_in ()
{
	session->set_punch_in (!session->get_punch_in());
}

void
BasicUI::toggle_punch_out ()
{
	session->set_punch_out (!session->get_punch_out());
}

bool
BasicUI::get_record_enabled ()
{
	return session->get_record_enabled();
}

void
BasicUI::set_record_enable (bool yn)
{
	if (yn) {
		session->maybe_enable_record ();
	} else {
		session->disable_record (false, true);
	}
}

jack_nframes_t
BasicUI::transport_frame ()
{
	return session->transport_frame();
}

void
BasicUI::locate (jack_nframes_t where, bool roll_after_locate)
{
	session->request_locate (where, roll_after_locate);
}

bool
BasicUI::locating ()
{
	return session->locate_pending();
}

bool
BasicUI::locked ()
{
	return session->transport_locked ();
}

jack_nframes_t
BasicUI::smpte_frames_per_hour ()
{
	return session->smpte_frames_per_hour ();
}

void
BasicUI::smpte_time (jack_nframes_t where, SMPTE_t& smpte)
{
	session->smpte_time (where, *((SMPTE_Time *) &smpte));
}

void 
BasicUI::smpte_to_sample (SMPTE_t& smpte, jack_nframes_t& sample, bool use_offset, bool use_subframes) const
{
	session->smpte_to_sample (*((SMPTE_Time*)&smpte), sample, use_offset, use_subframes);
}

void 
BasicUI::sample_to_smpte (jack_nframes_t sample, SMPTE_t& smpte, bool use_offset, bool use_subframes) const
{
	session->sample_to_smpte (sample, *((SMPTE_Time*)&smpte), use_offset, use_subframes);
}