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

    $Id$
*/

#include <sigc++/rettype.h>
#include <cstdlib>
#include <cmath>

#include <libgnomecanvas/libgnomecanvas.h>
#include <gtkmm2ext/gtk_ui.h>

#include <ardour/location.h>

#include "editor.h"
#include "marker.h"
#include "selection.h"
#include "editing.h"
#include "gui_thread.h"

#include "i18n.h"

using namespace std;
using namespace sigc;
using namespace ARDOUR;
using namespace Gtk;

void
Editor::clear_marker_display ()
{
	for (LocationMarkerMap::iterator i = location_markers.begin(); i != location_markers.end(); ++i) {
		delete i->second;
	}

	location_markers.clear ();
}

void
Editor::add_new_location (Location *location)
{
	LocationMarkers *lam = new LocationMarkers;
	uint32_t color;

	if (location->is_cd_marker()) {
		color = location_cd_marker_color;
	} else if (location->is_mark()) {
		color = location_marker_color;
	} else if (location->is_auto_loop()) {
		color = location_loop_color;
	} else if (location->is_auto_punch()) {
		color = location_punch_color;
	} else {
		color = location_range_color;
	}

	if (location->is_mark()) {
		lam->start = new Marker (*this, GNOME_CANVAS_GROUP(marker_group), color, 
					 location->name(), Marker::Mark, PublicEditor::canvas_marker_event, location->start());
		lam->end   = 0;

	} else if (location->is_auto_loop()) {
		// transport marker
		lam->start = new Marker (*this, GNOME_CANVAS_GROUP(transport_marker_group), color, 
					 location->name(), Marker::LoopStart, PublicEditor::canvas_marker_event, location->start());
		lam->end   = new Marker (*this, GNOME_CANVAS_GROUP(transport_marker_group), color, 
					 location->name(), Marker::LoopEnd, PublicEditor::canvas_marker_event, location->end());
		
	} else if (location->is_auto_punch()) {
		// transport marker
		lam->start = new Marker (*this, GNOME_CANVAS_GROUP(transport_marker_group), color, 
					 location->name(), Marker::PunchIn, PublicEditor::canvas_marker_event, location->start());
		lam->end   = new Marker (*this, GNOME_CANVAS_GROUP(transport_marker_group), color, 
					 location->name(), Marker::PunchOut, PublicEditor::canvas_marker_event, location->end());
		
	} else {

		// range marker
		lam->start = new Marker (*this, GNOME_CANVAS_GROUP(range_marker_group), color, 
					 location->name(), Marker::Start, PublicEditor::canvas_marker_event, location->start());
		lam->end   = new Marker (*this, GNOME_CANVAS_GROUP(range_marker_group), color, 
					 location->name(), Marker::End, PublicEditor::canvas_marker_event, location->end());
	}

	if (location->is_hidden ()) {
		lam->hide();
	} else {
		lam->show ();
	}

	location->start_changed.connect (mem_fun(*this, &Editor::location_changed));
	location->end_changed.connect (mem_fun(*this, &Editor::location_changed));
	location->changed.connect (mem_fun(*this, &Editor::location_changed));
	location->name_changed.connect (mem_fun(*this, &Editor::location_changed));
	location->FlagsChanged.connect (mem_fun(*this, &Editor::location_flags_changed));

	pair<Location*,LocationMarkers*> newpair;

	newpair.first = location;
	newpair.second = lam;

	location_markers.insert (newpair);
}

void
Editor::location_changed (Location *location)
{
	ENSURE_GUI_THREAD (bind (mem_fun(*this, &Editor::location_changed), location));

	LocationMarkers *lam = find_location_markers (location);

	if (lam == 0) {
		/* a location that isn't "marked" with markers */
		return;
	}
	
	lam->set_name (location->name());
	lam->set_position (location->start(), location->end());

	if (location->is_auto_loop()) {
		update_loop_range_view ();
	} else if (location->is_auto_punch()) {
		update_punch_range_view ();
	}
}

void
Editor::location_flags_changed (Location *location, void *src)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Editor::location_flags_changed), location, src));
	
	LocationMarkers *lam = find_location_markers (location);
	
	if (lam == 0) {
		/* a location that isn't "marked" with markers */
		return;
	}

	if (location->is_cd_marker()) {
		lam->set_color_rgba (location_cd_marker_color);
	} else if (location->is_mark()) {
		lam->set_color_rgba (location_marker_color);
	} else if (location->is_auto_punch()) {
		lam->set_color_rgba (location_punch_color);
	} else if (location->is_auto_loop()) {
		lam->set_color_rgba (location_loop_color);
	} else {
		lam->set_color_rgba (location_range_color);
	}
	
	if (location->is_hidden()) {
		lam->hide();
	} else {
		lam->show ();
	}
}

Editor::LocationMarkers::~LocationMarkers ()
{
	if (start) {
		delete start;
	}

	if (end) {
		delete end;
	}
}

Editor::LocationMarkers *
Editor::find_location_markers (Location *location)
{
	LocationMarkerMap::iterator i;

	for (i = location_markers.begin(); i != location_markers.end(); ++i) {
		if ((*i).first == location) {
			return (*i).second;
		}
	}

	return 0;
}

Location *
Editor::find_location_from_marker (Marker *marker, bool& is_start)
{
	LocationMarkerMap::iterator i;

	for (i = location_markers.begin(); i != location_markers.end(); ++i) {
		LocationMarkers *lm = (*i).second;
		if (lm->start == marker) {
			is_start = true;
			return (*i).first;
		} else if (lm->end == marker) {
			is_start = false;
			return (*i).first;
		}
	}

	return 0;
}

void
Editor::refresh_location_display_internal (Locations::LocationList& locations)
{
	clear_marker_display ();
	
	for (Locations::LocationList::iterator i = locations.begin(); i != locations.end(); ++i) {
		add_new_location (*i);
	}
}

void
Editor::refresh_location_display ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &Editor::refresh_location_display));
	
	if (session) {
		session->locations()->apply (*this, &Editor::refresh_location_display_internal);
	}
}

void
Editor::refresh_location_display_s (Change ignored)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Editor::refresh_location_display_s), ignored));

	if (session) {
		session->locations()->apply (*this, &Editor::refresh_location_display_internal);
	}
}

void
Editor::LocationMarkers::hide() 
{
	start->hide ();
	if (end) { end->hide(); }
}

void
Editor::LocationMarkers::show() 
{
	start->show ();
	if (end) { end->show(); }
}

void
Editor::LocationMarkers::set_name (const string& str) 
{
	start->set_name (str);
	if (end) { end->set_name (str); }
}

void
Editor::LocationMarkers::set_position (jack_nframes_t startf, 
				       jack_nframes_t endf) 
{
	start->set_position (startf);
	if (end) { end->set_position (endf); }
}

void
Editor::LocationMarkers::set_color_rgba (uint32_t rgba) 
{
	start->set_color_rgba (rgba);
	if (end) { end->set_color_rgba (rgba); }
}

void
Editor::mouse_add_new_marker (jack_nframes_t where)
{
	if (session) {
		Location *location = new Location (where, where, "mark", Location::IsMark);
		session->begin_reversible_command (_("add marker"));
		session->add_undo (session->locations()->get_memento());
		session->locations()->add (location, true);
		session->add_redo_no_execute (session->locations()->get_memento());
		session->commit_reversible_command ();
	}
}

void
Editor::remove_marker (GnomeCanvasItem* item, GdkEvent* event)
{
	Marker* marker;
	bool is_start;

	if ((marker = static_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* loc = find_location_from_marker (marker, is_start);

	if (session && loc) {
		if (loc->is_end()) {
			/* you can't hide or delete this marker */
			return;
		}
		if (loc->is_auto_loop() || loc->is_auto_punch()) {
			// just hide them
			loc->set_hidden (true, this);
		}
		else {
			Gtk::Main::idle.connect (bind (mem_fun(*this, &Editor::really_remove_marker), loc));
		}
	}
}

gint
Editor::really_remove_marker (Location* loc)
{
	session->begin_reversible_command (_("remove marker"));
	session->add_undo (session->locations()->get_memento());
	session->locations()->remove (loc);
	session->add_redo_no_execute (session->locations()->get_memento());
	session->commit_reversible_command ();
	return FALSE;
}

void
Editor::location_gone (Location *location)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Editor::location_gone), location));
	
	LocationMarkerMap::iterator i;

	if (location == transport_loop_location()) {
		update_loop_range_view (true);
	}

	if (location == transport_punch_location()) {
		update_punch_range_view (true);
	}
	
	for (i = location_markers.begin(); i != location_markers.end(); ++i) {
		if ((*i).first == location) {
			delete (*i).second;
			location_markers.erase (i);
			break;
		}
	}
}

void
Editor::tm_marker_context_menu (GdkEventButton* ev, GnomeCanvasItem* item)
{
	if (tm_marker_menu == 0) {
		build_tm_marker_menu ();
	}

	marker_menu_item = item;
	tm_marker_menu->popup (1, ev->time);

}


void
Editor::marker_context_menu (GdkEventButton* ev, GnomeCanvasItem* item)
{
	Marker * marker;
	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}
	
	bool is_start;
	Location * loc = find_location_from_marker (marker, is_start);
	if (loc == transport_loop_location() || loc == transport_punch_location()) {
		if (transport_marker_menu == 0) {
			build_transport_marker_menu ();
		}
		marker_menu_item = item;
		transport_marker_menu->popup (1, ev->time);
	}
	else {
		if (marker_menu == 0) {
			build_marker_menu ();
		}

		Menu_Helpers::MenuList & children = marker_menu->items();
		// XXX: should really find this some other way
		if (children.size() >= 3) {
			MenuItem * loopitem = children[2];
			if (loopitem) {
				if (loc->is_mark()) {
					loopitem->set_sensitive(false);
				}
				else {
					loopitem->set_sensitive(true);
				}
			}
		}
		
		marker_menu_item = item;
		marker_menu->popup (1, ev->time);
	}
}


void
Editor::new_transport_marker_context_menu (GdkEventButton* ev, GnomeCanvasItem* item)
{
	if (new_transport_marker_menu == 0) {
		build_new_transport_marker_menu ();
	}

	new_transport_marker_menu->popup (1, ev->time);

}

void
Editor::transport_marker_context_menu (GdkEventButton* ev, GnomeCanvasItem* item)
{
	if (transport_marker_menu == 0) {
		build_transport_marker_menu ();
	}

	transport_marker_menu->popup (1, ev->time);
}

void
Editor::build_marker_menu ()
{
	using namespace Menu_Helpers;

	marker_menu = new Menu;
	MenuList& items = marker_menu->items();
	marker_menu->set_name ("ArdourContextMenu");

	items.push_back (MenuElem (_("Locate to"), mem_fun(*this, &Editor::marker_menu_set_playhead)));
	items.push_back (MenuElem (_("Play from"), mem_fun(*this, &Editor::marker_menu_play_from)));
	items.push_back (MenuElem (_("Loop range"), mem_fun(*this, &Editor::marker_menu_loop_range)));
	items.push_back (MenuElem (_("Set from playhead"), mem_fun(*this, &Editor::marker_menu_set_from_playhead)));
	items.push_back (MenuElem (_("Set from range"), mem_fun(*this, &Editor::marker_menu_set_from_selection)));

	items.push_back (SeparatorElem());

	items.push_back (MenuElem (_("Rename"), mem_fun(*this, &Editor::marker_menu_rename)));
	items.push_back (MenuElem (_("Hide"), mem_fun(*this, &Editor::marker_menu_hide)));
	items.push_back (MenuElem (_("Remove"), mem_fun(*this, &Editor::marker_menu_remove)));
}

void
Editor::build_tm_marker_menu ()
{
	using namespace Menu_Helpers;

	tm_marker_menu = new Menu;
	MenuList& items = tm_marker_menu->items();
	tm_marker_menu->set_name ("ArdourContextMenu");

	items.push_back (MenuElem (_("Edit"), mem_fun(*this, &Editor::marker_menu_edit)));
	items.push_back (MenuElem (_("Remove"), mem_fun(*this, &Editor::marker_menu_remove)));
}

void
Editor::build_new_transport_marker_menu ()
{
	using namespace Menu_Helpers;

	new_transport_marker_menu = new Menu;
	MenuList& items = new_transport_marker_menu->items();
	new_transport_marker_menu->set_name ("ArdourContextMenu");

	items.push_back (MenuElem (_("Set Loop Range"), mem_fun(*this, &Editor::new_transport_marker_menu_set_loop)));
	items.push_back (MenuElem (_("Set Punch Range"), mem_fun(*this, &Editor::new_transport_marker_menu_set_punch)));

	new_transport_marker_menu->signal_unmap_event().connect ( mem_fun(*this, &Editor::new_transport_marker_menu_popdown)); 
}

void
Editor::build_transport_marker_menu ()
{
	using namespace Menu_Helpers;

	transport_marker_menu = new Menu;
	MenuList& items = transport_marker_menu->items();
	transport_marker_menu->set_name ("ArdourContextMenu");

	items.push_back (MenuElem (_("Locate to"), mem_fun(*this, &Editor::marker_menu_set_playhead)));
	items.push_back (MenuElem (_("Play from"), mem_fun(*this, &Editor::marker_menu_play_from)));
	items.push_back (MenuElem (_("Set from playhead"), mem_fun(*this, &Editor::marker_menu_set_from_playhead)));
	items.push_back (MenuElem (_("Set from range"), mem_fun(*this, &Editor::marker_menu_set_from_selection)));
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Hide"), mem_fun(*this, &Editor::marker_menu_hide)));
}

void
Editor::marker_menu_hide ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {
		l->set_hidden (true, this);
	}
}

void
Editor::marker_menu_play_from ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {

		if (l->is_mark()) {
			session->request_locate (l->start(), true);
		}
		else {
			//session->request_bounded_roll (l->start(), l->end());
			
			if (is_start) {
				session->request_locate (l->start(), true);
			} else {
				session->request_locate (l->end(), true);
			}
		}
	}
}

void
Editor::marker_menu_set_playhead ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {

		if (l->is_mark()) {
			session->request_locate (l->start(), false);
		}
		else {
			if (is_start) {
				session->request_locate (l->start(), false);
			} else {
				session->request_locate (l->end(), false);
			}
		}
	}
}

void
Editor::marker_menu_set_from_playhead ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {

		if (l->is_mark()) {
			l->set_start (session->transport_frame ());
		}
		else {
			if (is_start) {
				l->set_start (session->transport_frame ());
			} else {
				l->set_end (session->transport_frame ());
			}
		}
	}
}

void
Editor::marker_menu_set_from_selection ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {

		if (l->is_mark()) {
			// nothing for now
		}
		else {

			/* if range selection use first to last */

			if (mouse_mode == Editing::MouseRange) {
				if (!selection->time.empty()) {
					l->set_start (selection->time.start());
					l->set_end (selection->time.end_frame());
				}
			}
			else {
				if (!selection->audio_regions.empty()) {
					l->set_start (selection->audio_regions.start());
					l->set_end (selection->audio_regions.end_frame());
				}
			}
		}
	}
}

void
Editor::marker_menu_loop_range ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* l;
	bool is_start;
	
	if ((l = find_location_from_marker (marker, is_start)) != 0) {
		Location* l2;
		if ((l2 = transport_loop_location()) != 0) {
			l2->set (l->start(), l->end());
			
			// enable looping, reposition and start rolling
			session->request_auto_loop(true);
			session->request_locate (l2->start(), true);
		}
	}
}

void
Editor::marker_menu_edit ()
{
	MeterMarker* mm;
	TempoMarker* tm;
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	if ((mm = dynamic_cast<MeterMarker*> (marker)) != 0) {
		edit_meter_section (&mm->meter());
	} else if ((tm = dynamic_cast<TempoMarker*> (marker)) != 0) {
		edit_tempo_section (&tm->tempo());
	} else {
		fatal << X_("programming erorr: unhandled marker type in Editor::marker_menu_edit")
		      << endmsg;
		/*NOTREACHED*/
	}
}

void
Editor::marker_menu_remove ()
{
	MeterMarker* mm;
	TempoMarker* tm;
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	if ((mm = dynamic_cast<MeterMarker*> (marker)) != 0) {
		remove_meter_marker (marker_menu_item);
	} else if ((tm = dynamic_cast<TempoMarker*> (marker)) != 0) {
		remove_tempo_marker (marker_menu_item);
	} else {
		remove_marker (marker_menu_item, (GdkEvent*) 0);
	}
}

void
Editor::marker_menu_rename ()
{
	Marker* marker;

	if ((marker = reinterpret_cast<Marker *> (gtk_object_get_data (GTK_OBJECT(marker_menu_item), "marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		/*NOTREACHED*/
	}

	Location* loc;
	bool is_start;

	loc = find_location_from_marker (marker, is_start);

	if (!loc) return;
	
	Dialog dialog;
	Entry  entry;
	Button ok_button (_("OK"));
	Button cancel_button (_("Cancel"));
	
	if (loc->is_mark()) {
		dialog.set_title (_("ardour: rename mark"));
	} else {
		dialog.set_title (_("ardour: rename range"));
	}

	dialog.set_name ("MarkRenameWindow");
	dialog.set_size_request (300, -1);
	dialog.set_position (Gtk::WIN_POS_MOUSE);
	dialog.set_modal (true);

	dialog.get_vbox()->set_border_width (10);
	dialog.get_vbox()->pack_start (entry);
	dialog.get_action_area()->pack_start (ok_button);
	dialog.get_action_area()->pack_start (cancel_button);

	entry.set_text (loc->name());
	entry.set_name ("MarkerNameDisplay");
	ok_button.set_name ("EditorGTKButton");
	cancel_button.set_name ("EditorGTKButton");

	entry.activate.connect (bind (mem_fun(*this, &Editor::finish_sub_event_loop), 1));
	cancel_button.signal_clicked().connect (bind (mem_fun(*this, &Editor::finish_sub_event_loop), -1));
	ok_button.signal_clicked().connect (bind (mem_fun(*this, &Editor::finish_sub_event_loop), 1));
	dialog.signal_delete_event().connect (bind (mem_fun(*this, &Editor::finish_sub_event_loop_on_delete), -1));

	dialog.show_all ();
	entry.grab_focus ();

	run_sub_event_loop ();

	if (sub_event_loop_status == 1) {

		Location* l;
		bool is_start;

		if ((l = find_location_from_marker (marker, is_start)) != 0) {
			l->set_name (entry.get_text());
		}
	}
}

gint
Editor::new_transport_marker_menu_popdown (GdkEventAny *ev)
{
	// hide rects
	gnome_canvas_item_hide (transport_bar_drag_rect);
	gnome_canvas_item_hide (range_marker_drag_rect);

	return FALSE;
}

void
Editor::new_transport_marker_menu_set_loop ()
{
	if (!session) return;
	
	begin_reversible_command (_("set loop range"));
	
	Location* tll;

	if ((tll = transport_loop_location()) == 0) {
		Location* loc = new Location (temp_location->start(), temp_location->end(), _("Loop"),  Location::IsAutoLoop);
		session->add_undo (session->locations()->get_memento());
		session->locations()->add (loc, true);
		session->set_auto_loop_location (loc);
		session->add_redo_no_execute (session->locations()->get_memento());
	}
	else {
		session->add_undo (rettype<void>(bind (mem_fun (*tll, &Location::set), tll->start(), tll->end())));
		session->add_redo (rettype<void>(bind (mem_fun (*tll, &Location::set), temp_location->start(), temp_location->end())));
		tll->set_hidden (false, this);
		tll->set (temp_location->start(), temp_location->end());
	}
	
	commit_reversible_command ();
}

void
Editor::new_transport_marker_menu_set_punch ()
{
	if (!session) return;
	
	begin_reversible_command (_("set punch range"));
	
	Location* tpl;

	if ((tpl = transport_punch_location()) == 0) {
		tpl = new Location (temp_location->start(), temp_location->end(), _("Punch"), Location::IsAutoPunch);
		session->add_undo (session->locations()->get_memento());
		session->locations()->add (tpl, true);
		session->set_auto_punch_location (tpl);
		session->add_redo_no_execute (session->locations()->get_memento());
	} else {
		session->add_undo (rettype<void>(bind (mem_fun (*tpl, &Location::set), tpl->start(), tpl->end())));
		session->add_redo (rettype<void>(bind (mem_fun (*tpl, &Location::set), temp_location->start(), temp_location->end())));
		tpl->set_hidden(false, this);
		tpl->set(temp_location->start(), temp_location->end());
	}
	
	commit_reversible_command ();
}

void
Editor::update_loop_range_view (bool visibility)
{
	if (session == 0) {
		return;
	}

	Location* tll;

	if (session->get_auto_loop() && ((tll = transport_loop_location()) != 0)) {

		double x1 = frame_to_pixel (tll->start());
		double x2 = frame_to_pixel (tll->end());
		
		gnome_canvas_item_set (transport_loop_range_rect, "x1", x1, "x2", x2, NULL);
		
		if (visibility) {
			gnome_canvas_item_show (transport_loop_range_rect);
		}
	}
	else if (visibility) {
		gnome_canvas_item_hide (transport_loop_range_rect);
	}
}

void
Editor::update_punch_range_view (bool visibility)
{
	if (session == 0) {
		return;
	}

	Location* tpl;

	if ((session->get_punch_in() || session->get_punch_out()) && ((tpl = transport_punch_location()) != 0)) {

		double x1 = frame_to_pixel (tpl->start());
		double x2 = frame_to_pixel (tpl->end());
		
		gnome_canvas_item_set (transport_punch_range_rect, "x1", x1, "x2", x2, NULL);
		
		if (visibility) {
			gnome_canvas_item_show (transport_punch_range_rect);
		}
	}
	else if (visibility) {
		gnome_canvas_item_hide (transport_punch_range_rect);
	}

// 	if (session->get_punch_in()) {
// 		double x = frame_to_pixel (transport_punch_location->start());
// 		gnome_canvas_item_set (transport_punchin_line, "x1", x, "x2", x, NULL);
		
// 		if (visibility) {
// 			gnome_canvas_item_show (transport_punchin_line);
// 		}
// 	}
// 	else if (visibility) {
// 		gnome_canvas_item_hide (transport_punchin_line);
// 	}
	
// 	if (session->get_punch_out()) {
// 		double x = frame_to_pixel (transport_punch_location->end());
		
// 		gnome_canvas_item_set (transport_punchout_line, "x1", x, "x2", x, NULL);
		
// 		if (visibility) {
// 			gnome_canvas_item_show (transport_punchout_line);
// 		}
// 	}
// 	else if (visibility) {
// 		gnome_canvas_item_hide (transport_punchout_line);
// 	}
}
