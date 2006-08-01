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

#include <algorithm>
#include <string>
#include <vector>

#include <sigc++/bind.h>

#include <pbd/error.h>
#include <pbd/stl_delete.h>
#include <pbd/whitespace.h>

#include <gtkmm2ext/gtk_ui.h>
#include <gtkmm2ext/selector.h>
#include <gtkmm2ext/stop_signal.h>
#include <gtkmm2ext/bindable_button.h>
#include <gtkmm2ext/utils.h>

#include <ardour/midi_playlist.h>
#include <ardour/midi_diskstream.h>
#include <ardour/insert.h>
#include <ardour/ladspa_plugin.h>
#include <ardour/location.h>
#include <ardour/playlist.h>
#include <ardour/session.h>
#include <ardour/session_playlist.h>
#include <ardour/utils.h>

#include "ardour_ui.h"
#include "midi_time_axis.h"
#include "automation_time_axis.h"
#include "canvas_impl.h"
#include "crossfade_view.h"
#include "enums.h"
#include "gui_thread.h"
#include "keyboard.h"
#include "playlist_selector.h"
#include "plugin_selector.h"
#include "plugin_ui.h"
#include "point_selection.h"
#include "prompter.h"
#include "public_editor.h"
#include "redirect_automation_line.h"
#include "redirect_automation_time_axis.h"
#include "region_view.h"
#include "rgb_macros.h"
#include "selection.h"
#include "simplerect.h"
#include "midi_streamview.h"
#include "utils.h"

#include <ardour/midi_track.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Editing;


MidiTimeAxisView::MidiTimeAxisView (PublicEditor& ed, Session& sess, boost::shared_ptr<Route> rt, Canvas& canvas)
	: AxisView(sess), // FIXME: won't compile without this, why??
	RouteTimeAxisView(ed, sess, rt, canvas)
{
	subplugin_menu.set_name ("ArdourContextMenu");

	_view = new MidiStreamView (*this);

	ignore_toggle = false;

	mute_button->set_active (false);
	solo_button->set_active (false);
	
	if (is_midi_track())
		controls_ebox.set_name ("MidiTimeAxisViewControlsBaseUnselected");
	else // bus
		controls_ebox.set_name ("MidiBusControlsBaseUnselected");

	/* map current state of the route */

	//redirects_changed (0);

	ensure_xml_node ();

	set_state (*xml_node);
	
	//_route.redirects_changed.connect (mem_fun(*this, &MidiTimeAxisView::redirects_changed));

	if (is_midi_track()) {

		controls_ebox.set_name ("MidiTrackControlsBaseUnselected");
		controls_base_selected_name = "MidiTrackControlsBaseSelected";
		controls_base_unselected_name = "MidiTrackControlsBaseUnselected";

		/* ask for notifications of any new RegionViews */
		//view->MidiRegionViewAdded.connect (mem_fun(*this, &MidiTimeAxisView::region_view_added));
		//view->attach ();

	} else { /* bus */

		controls_ebox.set_name ("MidiBusControlsBaseUnselected");
		controls_base_selected_name = "MidiBusControlsBaseSelected";
		controls_base_unselected_name = "MidiBusControlsBaseUnselected";
	}
}

MidiTimeAxisView::~MidiTimeAxisView ()
{
}

guint32
MidiTimeAxisView::show_at (double y, int& nth, Gtk::VBox *parent)
{
	ensure_xml_node ();
	xml_node->add_property ("shown_editor", "yes");
		
	return TimeAxisView::show_at (y, nth, parent);
}

void
MidiTimeAxisView::hide ()
{
	ensure_xml_node ();
	xml_node->add_property ("shown_editor", "no");

	TimeAxisView::hide ();
}

void
MidiTimeAxisView::set_state (const XMLNode& node)
{
	const XMLProperty *prop;
	
	TimeAxisView::set_state (node);
	
	if ((prop = node.property ("shown_editor")) != 0) {
		if (prop->value() == "no") {
			_marked_for_display = false;
		} else {
			_marked_for_display = true;
		}
	} else {
		_marked_for_display = true;
	}
	
	XMLNodeList nlist = node.children();
	XMLNodeConstIterator niter;
	XMLNode *child_node;
	
	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		child_node = *niter;
		
		// uh... do stuff..
	}
}

void
MidiTimeAxisView::build_display_menu ()
{
	using namespace Menu_Helpers;

	/* get the size menu ready */

	build_size_menu ();

	/* prepare it */

	TimeAxisView::build_display_menu ();

	/* now fill it with our stuff */

	MenuList& items = display_menu->items();
	display_menu->set_name ("ArdourContextMenu");
	
	items.push_back (MenuElem (_("Height"), *size_menu));
	items.push_back (MenuElem (_("Color"), mem_fun(*this, &MidiTimeAxisView::select_track_color)));


	items.push_back (SeparatorElem());

	build_remote_control_menu ();
	items.push_back (MenuElem (_("Remote Control ID"), *remote_control_menu));

	automation_action_menu = manage (new Menu);
	MenuList& automation_items = automation_action_menu->items();
	automation_action_menu->set_name ("ArdourContextMenu");
	
	automation_items.push_back (SeparatorElem());

	automation_items.push_back (MenuElem (_("Plugins"), subplugin_menu));

	if (is_midi_track()) {

		Menu* alignment_menu = manage (new Menu);
		MenuList& alignment_items = alignment_menu->items();
		alignment_menu->set_name ("ArdourContextMenu");

		RadioMenuItem::Group align_group;
		
		alignment_items.push_back (RadioMenuElem (align_group, _("Align with existing material"), bind (mem_fun(*this, &MidiTimeAxisView::set_align_style), ExistingMaterial)));
		align_existing_item = dynamic_cast<RadioMenuItem*>(&alignment_items.back());
		if (get_diskstream()->alignment_style() == ExistingMaterial) {
			align_existing_item->set_active();
		}
		alignment_items.push_back (RadioMenuElem (align_group, _("Align with capture time"), bind (mem_fun(*this, &MidiTimeAxisView::set_align_style), CaptureTime)));
		align_capture_item = dynamic_cast<RadioMenuItem*>(&alignment_items.back());
		if (get_diskstream()->alignment_style() == CaptureTime) {
			align_capture_item->set_active();
		}
		
		items.push_back (MenuElem (_("Alignment"), *alignment_menu));

		get_diskstream()->AlignmentStyleChanged.connect (mem_fun(*this, &MidiTimeAxisView::align_style_changed));
	}

	items.push_back (SeparatorElem());
	items.push_back (CheckMenuElem (_("Active"), mem_fun(*this, &RouteUI::toggle_route_active)));
	route_active_menu_item = dynamic_cast<CheckMenuItem *> (&items.back());
	route_active_menu_item->set_active (_route->active());

	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Remove"), mem_fun(*this, &RouteUI::remove_this_route)));

}

// FIXME: duplicated in audio_time_axis.cc
/*static string 
legalize_for_xml_node (string str)
{
	string::size_type pos;
	string legal_chars = "abcdefghijklmnopqrtsuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_+=:";
	string legal;

	legal = str;
	pos = 0;

	while ((pos = legal.find_first_not_of (legal_chars, pos)) != string::npos) {
		legal.replace (pos, 1, "_");
		pos += 1;
	}

	return legal;
}*/

void
MidiTimeAxisView::route_active_changed ()
{
	RouteUI::route_active_changed ();

	if (is_midi_track()) {
		if (_route->active()) {
			controls_ebox.set_name ("MidiTrackControlsBaseUnselected");
			controls_base_selected_name = "MidiTrackControlsBaseSelected";
			controls_base_unselected_name = "MidiTrackControlsBaseUnselected";
		} else {
			controls_ebox.set_name ("MidiTrackControlsBaseInactiveUnselected");
			controls_base_selected_name = "MidiTrackControlsBaseInactiveSelected";
			controls_base_unselected_name = "MidiTrackControlsBaseInactiveUnselected";
		}
	} else {
		if (_route->active()) {
			controls_ebox.set_name ("BusControlsBaseUnselected");
			controls_base_selected_name = "BusControlsBaseSelected";
			controls_base_unselected_name = "BusControlsBaseUnselected";
		} else {
			controls_ebox.set_name ("BusControlsBaseInactiveUnselected");
			controls_base_selected_name = "BusControlsBaseInactiveSelected";
			controls_base_unselected_name = "BusControlsBaseInactiveUnselected";
		}
	}
}

XMLNode* 
MidiTimeAxisView::get_child_xml_node (const string & childname)
{
	return RouteUI::get_child_xml_node (childname);
}

