/*
    Copyright (C) 2000 Paul Davis 

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

#include <cstdlib>
#include <cmath>

#include <string>

#include <pbd/error.h>
#include <pbd/pthread_utils.h>
#include <pbd/memento_command.h>

#include <gtkmm2ext/window_title.h>

#include "editor.h"
#include "audio_time_axis.h"
#include "audio_region_view.h"
#include "region_selection.h"

#include <ardour/session.h>
#include <ardour/region.h>
#include <ardour/audioplaylist.h>
#include <ardour/audio_track.h>
#include <ardour/audioregion.h>
#include <ardour/audio_diskstream.h>
#include <ardour/stretch.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace sigc;
using namespace Gtk;
using namespace Gtkmm2ext;

Editor::TimeStretchDialog::TimeStretchDialog (Editor& e)
	: ArdourDialog ("time stretch dialog"),
	  editor (e),
	  quick_button (_("Quick but Ugly")),
	  antialias_button (_("Skip Anti-aliasing"))
{
	set_modal (true);
	set_position (Gtk::WIN_POS_MOUSE);
	set_name (N_("TimeStretchDialog"));

	WindowTitle title(Glib::get_application_name());
	title += _("Timestretch");
	set_title(title.get_string());

	get_vbox()->set_spacing (5);
	get_vbox()->set_border_width (5);
	get_vbox()->pack_start (upper_button_box);
	get_vbox()->pack_start (progress_bar);

	upper_button_box.set_homogeneous (true);
	upper_button_box.set_spacing (5);
	upper_button_box.set_border_width (5);
	upper_button_box.pack_start (quick_button, true, true);
	upper_button_box.pack_start (antialias_button, true, true);

	action_button = add_button (_("Stretch/Shrink it"), Gtk::RESPONSE_ACCEPT);
	cancel_button = add_button (_("Cancel"), Gtk::RESPONSE_CANCEL);

	quick_button.set_name (N_("TimeStretchButton"));
	antialias_button.set_name (N_("TimeStretchButton"));
	progress_bar.set_name (N_("TimeStretchProgress"));

	show_all_children ();
}

gint
Editor::TimeStretchDialog::update_progress ()
{
	progress_bar.set_fraction (request.progress);
	return !request.done;
}

void
Editor::TimeStretchDialog::cancel_timestretch_in_progress ()
{
	status = -2;
	request.cancel = true;
	first_cancel.disconnect();
}

gint
Editor::TimeStretchDialog::delete_timestretch_in_progress (GdkEventAny* ev)
{
	status = -2;
	request.cancel = true;
	first_delete.disconnect();
	return TRUE;
}

int
Editor::run_timestretch (RegionSelection& regions, float fraction)
{
	if (current_timestretch == 0) {
		current_timestretch = new TimeStretchDialog (*this);
	}

	current_timestretch->progress_bar.set_fraction (0.0f);

	switch (current_timestretch->run ()) {
	case RESPONSE_ACCEPT:
		break;
	default:
		current_timestretch->hide ();
		return 1;
	}

	current_timestretch->status = 0;
	current_timestretch->regions = regions;
	current_timestretch->request.fraction = fraction;
	current_timestretch->request.quick_seek = current_timestretch->quick_button.get_active();
	current_timestretch->request.antialias = !current_timestretch->antialias_button.get_active();
	current_timestretch->request.progress = 0.0f;
	current_timestretch->request.done = false;
	current_timestretch->request.cancel = false;
	
	/* re-connect the cancel button and delete events */
	
	current_timestretch->first_cancel.disconnect();
	current_timestretch->first_delete.disconnect();
	
	current_timestretch->first_cancel = current_timestretch->cancel_button->signal_clicked().connect 
		(mem_fun (current_timestretch, &TimeStretchDialog::cancel_timestretch_in_progress));
	current_timestretch->first_delete = current_timestretch->signal_delete_event().connect 
		(mem_fun (current_timestretch, &TimeStretchDialog::delete_timestretch_in_progress));

	if (pthread_create_and_store ("timestretch", &current_timestretch->request.thread, 0, timestretch_thread, current_timestretch)) {
		current_timestretch->hide ();
		error << _("timestretch cannot be started - thread creation error") << endmsg;
		return -1;
	}

	pthread_detach (current_timestretch->request.thread);

	sigc::connection c = Glib::signal_timeout().connect (mem_fun (current_timestretch, &TimeStretchDialog::update_progress), 100);

	while (!current_timestretch->request.done) {
		gtk_main_iteration ();
	}

	c.disconnect ();
	
	current_timestretch->hide ();
	return current_timestretch->status;
}

void
Editor::do_timestretch (TimeStretchDialog& dialog)
{
	Track*    t;
	boost::shared_ptr<Playlist> playlist;
	boost::shared_ptr<Region>   new_region;

	for (RegionSelection::iterator i = dialog.regions.begin(); i != dialog.regions.end(); ) {
		AudioRegionView* arv = dynamic_cast<AudioRegionView*>(*i);
		if (!arv)
			continue;

		boost::shared_ptr<AudioRegion> region (arv->audio_region());
		TimeAxisView* tv = &(arv->get_time_axis_view());
		RouteTimeAxisView* rtv;
		RegionSelection::iterator tmp;
		
		tmp = i;
		++tmp;

		if ((rtv = dynamic_cast<RouteTimeAxisView*> (tv)) == 0) {
			i = tmp;
			continue;
		}

		if ((t = dynamic_cast<Track*> (rtv->route().get())) == 0) {
			i = tmp;
			continue;
		}
	
		if ((playlist = t->diskstream()->playlist()) == 0) {
			i = tmp;
			continue;
		}

		if (dialog.request.cancel) {
			/* we were cancelled */
			dialog.status = 1;
			return;
		}

		Stretch stretch (*session, dialog.request);

		if (stretch.run (region)) {
			dialog.status = -1;
			dialog.request.done = true;
			return;
		}

		if (!stretch.results.empty()) {
			new_region = stretch.results.front();

			XMLNode &before = playlist->get_state();
			playlist->replace_region (region, new_region, region->position());
			XMLNode &after = playlist->get_state();
			session->add_command (new MementoCommand<Playlist>(*playlist, &before, &after));
		}

		i = tmp;
	}

	dialog.status = 0;
	dialog.request.done = true;
}

void*
Editor::timestretch_thread (void *arg)
{
	PBD::ThreadCreated (pthread_self(), X_("TimeFX"));

	TimeStretchDialog* tsd = static_cast<TimeStretchDialog*>(arg);

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	tsd->editor.do_timestretch (*tsd);

	return 0;
}

