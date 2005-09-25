/*
    Copyright (C) 2003-2004 Paul Davis 

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

#include <gtkmm2ext/utils.h>
#include <ardour/audioengine.h>

#include "editor.h"
#include "mixer_strip.h"
#include "ardour_ui.h"
#include "selection.h"
#include "audio_time_axis.h"

#include "i18n.h"

void
Editor::editor_mixer_button_toggled ()
{
	show_editor_mixer (editor_mixer_button.get_active());
}

void
Editor::cms_deleted ()
{
	current_mixer_strip = 0;
}

void
Editor::show_editor_mixer (bool yn)
{
	if (yn) {

		if (current_mixer_strip == 0) {

			if (selection->tracks.empty()) {
				
				if (track_views.empty()) {
					return;
				}
				
				for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
					AudioTimeAxisView* atv;

					if ((atv = dynamic_cast<AudioTimeAxisView*> (*i)) != 0) {
						
						current_mixer_strip = new MixerStrip (*ARDOUR_UI::instance()->the_mixer(),
										      *session,
										      atv->route(), false);

						current_mixer_strip->GoingAway.connect (mem_fun(*this, &Editor::cms_deleted));						
						break;
					}
				}

			} else {
				for (TrackSelection::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
					AudioTimeAxisView* atv;

					if ((atv = dynamic_cast<AudioTimeAxisView*> (*i)) != 0) {

						current_mixer_strip = new MixerStrip (*ARDOUR_UI::instance()->the_mixer(),
										      *session,
										      atv->route(), false);
						current_mixer_strip->GoingAway.connect (mem_fun(*this, &Editor::cms_deleted));						
						break;
					}
				}

			}

			if (current_mixer_strip == 0) {
				return;
			}		
		}
		
		if (current_mixer_strip->get_parent() == 0) {

			current_mixer_strip->set_embedded (true);
			current_mixer_strip->Hiding.connect (mem_fun(*this, &Editor::current_mixer_strip_hidden));
			current_mixer_strip->GoingAway.connect (mem_fun(*this, &Editor::current_mixer_strip_removed));
			current_mixer_strip->set_width (editor_mixer_strip_width);
			current_mixer_strip->show_all ();
			
			global_hpacker.pack_start (*current_mixer_strip, false, false);
			global_hpacker.reorder_child (*current_mixer_strip, 0);
		}

	} else {

		if (current_mixer_strip) {
			editor_mixer_strip_width = current_mixer_strip->get_width ();
			if (current_mixer_strip->get_parent() != 0) {
				global_hpacker.remove (*current_mixer_strip);
			}
		}
	}
}

void
Editor::set_selected_mixer_strip (TimeAxisView& view)
{
	AudioTimeAxisView* at;
	bool show = false;

	if (!session || (at = dynamic_cast<AudioTimeAxisView*>(&view)) == 0) {
		return;
	}
	
	if (current_mixer_strip) {

		/* might be nothing to do */

		if (&current_mixer_strip->route() == &at->route()) {
			return;
		}

		if (current_mixer_strip->get_parent()) {
			show = true;
		}

		delete current_mixer_strip;
		current_mixer_strip = 0;
	}

	current_mixer_strip = new MixerStrip (*ARDOUR_UI::instance()->the_mixer(),
					      *session,
					      at->route());
	current_mixer_strip->GoingAway.connect (mem_fun(*this, &Editor::cms_deleted));
	
	if (show) {
		show_editor_mixer (true);
	}
}

void
Editor::update_current_screen ()
{
	if (session && engine.running()) {

		jack_nframes_t frame;

		frame = session->audible_frame();

		/* only update if the playhead is on screen or we are following it */

		if (_follow_playhead) {

			gnome_canvas_item_show (playhead_cursor->canvas_item);

			if (frame != last_update_frame) {

				if (frame < leftmost_frame || frame > leftmost_frame + current_page_frames()) {
					
					if (session->transport_speed() < 0) {
						if (frame > (current_page_frames()/2)) {
							center_screen (frame-(current_page_frames()/2));
						} else {
							center_screen (current_page_frames()/2);
						}
					} else {
						center_screen (frame+(current_page_frames()/2));
					}
				}

				playhead_cursor->set_position (frame);
			}

		} else {
			
			if (frame != last_update_frame) {
				if (frame < leftmost_frame || frame > leftmost_frame + current_page_frames()) {
					gnome_canvas_item_hide (playhead_cursor->canvas_item);
				} else {
					playhead_cursor->set_position (frame);
				}
			}
		}

		last_update_frame = frame;

		if (current_mixer_strip) {
			current_mixer_strip->fast_update ();
		}
		
	}
}

void
Editor::update_slower ()
{
		if (current_mixer_strip) {
			current_mixer_strip->update ();
		}
}

void
Editor::current_mixer_strip_removed ()
{
	if (current_mixer_strip) {
		/* it is being deleted */
		current_mixer_strip = 0;
	}
}

void
Editor::current_mixer_strip_hidden ()
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		
		AudioTimeAxisView* tmp;
		
		if ((tmp = dynamic_cast<AudioTimeAxisView*>(*i)) != 0) {
			if (&(tmp->route()) == &(current_mixer_strip->route())) {
				(*i)->set_selected (false);
				break;
			}
		}
	}
	global_hpacker.remove (*current_mixer_strip);
}

void
Editor::session_going_away ()
{
	for (vector<sigc::connection>::iterator i = session_connections.begin(); i != session_connections.end(); ++i) {
		(*i).disconnect ();
	}

	stop_scrolling ();
	selection->clear ();
	cut_buffer->clear ();

	clicked_regionview = 0;
	clicked_trackview = 0;
	clicked_audio_trackview = 0;
	clicked_crossfadeview = 0;
	entered_regionview = 0;
	entered_track = 0;
	latest_regionview = 0;
	region_list_display_drag_region = 0;
	last_update_frame = 0;
	drag_info.item = 0;
	last_audition_region = 0;
	region_list_button_region = 0;

	/* hide all tracks */

	hide_all_tracks (false);

	/* rip everything out of the list displays */

	region_list_clear (); // no clear() method in gtkmm 1.2 
	route_list.clear ();
	named_selection_display.clear ();
	edit_group_list.clear ();

	edit_cursor_clock.set_session (0);
	selection_start_clock.set_session (0);
	selection_end_clock.set_session (0);
	zoom_range_clock.set_session (0);
	nudge_clock.set_session (0);

	/* put editor/mixer toggle button in off position and disable until a new session is loaded */

	editor_mixer_button.set_active(false);
	editor_mixer_button.set_sensitive(false);
	/* clear tempo/meter rulers */

	remove_metric_marks ();
	hide_measures ();
	clear_marker_display ();

	if (current_bbt_points) {
		delete current_bbt_points;
		current_bbt_points = 0;
	}

	if (embed_audio_item) {
		embed_audio_item->set_sensitive (false);
	} 

	if (import_audio_item) {
		import_audio_item->set_sensitive (false);
	}

	/* mixer strip will be deleted all by itself 
	   when its route is deleted.
	*/

	current_mixer_strip = 0;

	set_title (_("ardour: editor"));

	session = 0;
}
