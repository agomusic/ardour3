/*
    Copyright (C) 2000-2004 Paul Davis 

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

#include <algorithm>
#include <sigc++/bind.h>

#include <gtkmm/accelmap.h>

#include <pbd/convert.h>
#include <glibmm/thread.h>

#include <gtkmm2ext/gtk_ui.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/stop_signal.h>
#include <gtkmm2ext/window_title.h>

#include <ardour/session.h>
#include <ardour/audio_track.h>
#include <ardour/session_route.h>
#include <ardour/audio_diskstream.h>
#include <ardour/plugin_manager.h>

#include "keyboard.h"
#include "mixer_ui.h"
#include "mixer_strip.h"
#include "plugin_selector.h"
#include "ardour_ui.h"
#include "prompter.h"
#include "utils.h"
#include "actions.h"
#include "gui_thread.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace sigc;
using namespace std;

using PBD::atoi;

Mixer_UI::Mixer_UI ()
	: Window (Gtk::WINDOW_TOPLEVEL)
{
	session = 0;
	_strip_width = Config->get_default_narrow_ms() ? Narrow : Wide;
	track_menu = 0;
	mix_group_context_menu = 0;
	no_track_list_redisplay = false;
	in_group_row_change = false;
	_visible = false;
	ignore_route_reorder = false;
	ignore_sync = false;

	Route::SyncOrderKeys.connect (mem_fun (*this, &Mixer_UI::sync_order_keys));

 	scroller_base.add_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
 	scroller_base.set_name ("MixerWindow");
 	scroller_base.signal_button_release_event().connect (mem_fun(*this, &Mixer_UI::strip_scroller_button_release));
	// add as last item of strip packer
	strip_packer.pack_end (scroller_base, true, true);

	scroller.add (strip_packer);
	scroller.set_policy (Gtk::POLICY_ALWAYS, Gtk::POLICY_AUTOMATIC);

	track_model = ListStore::create (track_columns);
	track_display.set_model (track_model);
	track_display.append_column (_("Strips"), track_columns.text);
	track_display.append_column (_("Show"), track_columns.visible);
	track_display.get_column (0)->set_data (X_("colnum"), GUINT_TO_POINTER(0));
	track_display.get_column (1)->set_data (X_("colnum"), GUINT_TO_POINTER(1));
	track_display.get_column (0)->set_expand(true);
	track_display.get_column (1)->set_expand(false);
	track_display.set_name (X_("MixerTrackDisplayList"));
	track_display.get_selection()->set_mode (Gtk::SELECTION_NONE);
	track_display.set_reorderable (true);
	track_display.set_headers_visible (true);

	track_model->signal_row_deleted().connect (mem_fun (*this, &Mixer_UI::track_list_delete));
	track_model->signal_row_changed().connect (mem_fun (*this, &Mixer_UI::track_list_change));
	track_model->signal_rows_reordered().connect (mem_fun (*this, &Mixer_UI::track_list_reorder));

	CellRendererToggle* track_list_visible_cell = dynamic_cast<CellRendererToggle*>(track_display.get_column_cell_renderer (1));
	track_list_visible_cell->property_activatable() = true;
	track_list_visible_cell->property_radio() = false;

	track_display.signal_button_press_event().connect (mem_fun (*this, &Mixer_UI::track_display_button_press), false);

	track_display_scroller.add (track_display);
	track_display_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	group_model = ListStore::create (group_columns);
	group_display.set_model (group_model);
	group_display.append_column (_("Group"), group_columns.text);
	group_display.append_column (_("Active"), group_columns.active);
	group_display.append_column (_("Show"), group_columns.visible);
	group_display.get_column (0)->set_data (X_("colnum"), GUINT_TO_POINTER(0));
	group_display.get_column (1)->set_data (X_("colnum"), GUINT_TO_POINTER(1));
	group_display.get_column (2)->set_data (X_("colnum"), GUINT_TO_POINTER(2));
	group_display.get_column (0)->set_expand(true);
	group_display.get_column (1)->set_expand(false);
	group_display.get_column (2)->set_expand(false);
	group_display.set_name ("MixerGroupList");
	group_display.get_selection()->set_mode (Gtk::SELECTION_SINGLE);
	group_display.set_reorderable (true);
	group_display.set_headers_visible (true);
	group_display.set_rules_hint (true);

	/* name is directly editable */

	CellRendererText* name_cell = dynamic_cast<CellRendererText*>(group_display.get_column_cell_renderer (0));
	name_cell->property_editable() = true;
	name_cell->signal_edited().connect (mem_fun (*this, &Mixer_UI::mix_group_name_edit));

	/* use checkbox for the active column */

	CellRendererToggle* active_cell = dynamic_cast<CellRendererToggle*>(group_display.get_column_cell_renderer (1));
	active_cell->property_activatable() = true;
	active_cell->property_radio() = false;

	/* use checkbox for the visible column */

	active_cell = dynamic_cast<CellRendererToggle*>(group_display.get_column_cell_renderer (2));
	active_cell->property_activatable() = true;
	active_cell->property_radio() = false;

	group_model->signal_row_changed().connect (mem_fun (*this, &Mixer_UI::mix_group_row_change));

	group_display.signal_button_press_event().connect (mem_fun (*this, &Mixer_UI::group_display_button_press), false);

	group_display_scroller.add (group_display);
	group_display_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	HBox* mix_group_display_button_box = manage (new HBox());

	Button* mix_group_add_button = manage (new Button ());
	Button* mix_group_remove_button = manage (new Button ());

	Widget* w;

	w = manage (new Image (Stock::ADD, ICON_SIZE_BUTTON));
	w->show();
	mix_group_add_button->add (*w);

	w = manage (new Image (Stock::REMOVE, ICON_SIZE_BUTTON));
	w->show();
	mix_group_remove_button->add (*w);

	mix_group_display_button_box->set_homogeneous (true);

	mix_group_add_button->signal_clicked().connect (mem_fun (*this, &Mixer_UI::new_mix_group));
	mix_group_remove_button->signal_clicked().connect (mem_fun (*this, &Mixer_UI::remove_selected_mix_group));

	mix_group_display_button_box->add (*mix_group_remove_button);
	mix_group_display_button_box->add (*mix_group_add_button);

	group_display_vbox.pack_start (group_display_scroller, true, true);
	group_display_vbox.pack_start (*mix_group_display_button_box, false, false);

	track_display_frame.set_name("BaseFrame");
	track_display_frame.set_shadow_type (Gtk::SHADOW_IN);
	track_display_frame.add(track_display_scroller);

	group_display_frame.set_name ("BaseFrame");
	group_display_frame.set_shadow_type (Gtk::SHADOW_IN);
	group_display_frame.add (group_display_vbox);

	rhs_pane1.pack1 (track_display_frame);
	rhs_pane1.pack2 (group_display_frame);

	list_vpacker.pack_start (rhs_pane1, true, true);

	global_hpacker.pack_start (scroller, true, true);
	global_hpacker.pack_start (out_packer, false, false, 12);

	list_hpane.add1(list_vpacker);
	list_hpane.add2(global_hpacker);

	rhs_pane1.signal_size_allocate().connect (bind (mem_fun(*this, &Mixer_UI::pane_allocation_handler), 
							static_cast<Gtk::Paned*> (&rhs_pane1)));
	list_hpane.signal_size_allocate().connect (bind (mem_fun(*this, &Mixer_UI::pane_allocation_handler), 
							 static_cast<Gtk::Paned*> (&list_hpane)));
	
	global_vpacker.pack_start (list_hpane, true, true);

	add (global_vpacker);
	set_name ("MixerWindow");
	
	WindowTitle title(Glib::get_application_name());
	title += _("Mixer");
	set_title (title.get_string());

	set_wmclass (X_("ardour_mixer"), "Ardour");

	add_accel_group (ActionManager::ui_manager->get_accel_group());

	signal_delete_event().connect (mem_fun (*this, &Mixer_UI::hide_window));
	add_events (Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK);

	_plugin_selector = new PluginSelector (PluginManager::the_manager());

	signal_configure_event().connect (mem_fun (*ARDOUR_UI::instance(), &ARDOUR_UI::configure_handler));

	_selection.RoutesChanged.connect (mem_fun(*this, &Mixer_UI::follow_strip_selection));

	mix_group_display_button_box->show();
	mix_group_add_button->show();
	mix_group_remove_button->show();

	global_hpacker.show();
	global_vpacker.show();
	scroller.show();
	scroller_base.show();
	scroller_hpacker.show();
	mixer_scroller_vpacker.show();
	list_vpacker.show();
	group_display_button_label.show();
	group_display_button.show();
	track_display_scroller.show();
	group_display_scroller.show();
	group_display_vbox.show();
	track_display_frame.show();
	group_display_frame.show();
	rhs_pane1.show();
	strip_packer.show();
	out_packer.show();
	list_hpane.show();
	track_display.show();
	group_display.show();

	auto_rebinding = FALSE;
}

Mixer_UI::~Mixer_UI ()
{
}

void
Mixer_UI::ensure_float (Window& win)
{
	win.set_transient_for (*this);
}

void
Mixer_UI::show_window ()
{
	present ();

	set_window_pos_and_size ();

	/* now reset each strips width so the right widgets are shown */
	MixerStrip* ms;

	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator ri;

	for (ri = rows.begin(); ri != rows.end(); ++ri) {
		ms = (*ri)[track_columns.strip];
		ms->set_width (ms->get_width(), ms->width_owner());
	}
	_visible = true;
}

bool
Mixer_UI::hide_window (GdkEventAny *ev)
{
	get_window_pos_and_size ();

	_visible = false;
	return just_hide_it(ev, static_cast<Gtk::Window *>(this));
}


void
Mixer_UI::add_strip (Session::RouteList& routes)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Mixer_UI::add_strip), routes));
	
	MixerStrip* strip;

	for (Session::RouteList::iterator x = routes.begin(); x != routes.end(); ++x) {
		boost::shared_ptr<Route> route = (*x);

		if (route->is_hidden()) {
			return;
		}
		
		strip = new MixerStrip (*this, *session, route);
		strips.push_back (strip);
		
		Config->get_default_narrow_ms() ? _strip_width = Narrow : _strip_width = Wide;

		Config->get_default_narrow_ms() ? _strip_width = Narrow : _strip_width = Wide;

		if (strip->width_owner() != strip) {
			strip->set_width (_strip_width, this);
		}

		show_strip (strip);
		
		no_track_list_redisplay = true;
		
		TreeModel::Row row = *(track_model->append());
		row[track_columns.text] = route->name();
		
		row[track_columns.visible] = strip->marked_for_display();
		row[track_columns.route] = route;
		row[track_columns.strip] = strip;
		
		strip->set_old_order_key (track_model->children().size() - 1);

		no_track_list_redisplay = false;
		redisplay_track_list ();
		
		route->NameChanged.connect (bind (mem_fun(*this, &Mixer_UI::strip_name_changed), strip));
		strip->GoingAway.connect (bind (mem_fun(*this, &Mixer_UI::remove_strip), strip));
		
		strip->signal_button_release_event().connect (bind (mem_fun(*this, &Mixer_UI::strip_button_release_event), strip));
	}
}

void
Mixer_UI::remove_strip (MixerStrip* strip)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Mixer_UI::remove_strip), strip));

	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator ri;
	list<MixerStrip *>::iterator i;

	if ((i = find (strips.begin(), strips.end(), strip)) != strips.end()) {
		strips.erase (i);
	}

	/* Decrement old order keys for strips `above' the one that is being removed */
	for (ri = rows.begin(); ri != rows.end(); ++ri) {
		MixerStrip* s = (*ri)[track_columns.strip];
		if (s->old_order_key() > strip->old_order_key()) {
			s->set_old_order_key (s->old_order_key() - 1);
		}
	}

	for (ri = rows.begin(); ri != rows.end(); ++ri) {
		if ((*ri)[track_columns.strip] == strip) {
			track_model->erase (ri);
			break;
		}
	}
}

const char*
Mixer_UI::get_order_key() 
{
	if (Config->get_sync_all_route_ordering()) {
		return X_("editor");
	} else {
		return X_("signal");
	}
}

void
Mixer_UI::sync_order_keys ()
{
	vector<int> neworder;
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator ri;

	if (ignore_sync || !session || (session->state_of_the_state() & Session::Loading) || rows.empty()) {
		return;
	}

	for (ri = rows.begin(); ri != rows.end(); ++ri) {
		neworder.push_back (0);
	}

	for (ri = rows.begin(); ri != rows.end(); ++ri) {
		boost::shared_ptr<Route> route = (*ri)[track_columns.route];
		MixerStrip* strip = (*ri)[track_columns.strip];
		neworder[route->order_key (get_order_key())] = strip->old_order_key ();
	}

	ignore_route_reorder = true;
	track_model->reorder (neworder);
	ignore_route_reorder = false;
}

void
Mixer_UI::follow_strip_selection ()
{
	for (list<MixerStrip *>::iterator i = strips.begin(); i != strips.end(); ++i) {
		(*i)->set_selected (_selection.selected ((*i)->route()));
	}
}

bool
Mixer_UI::strip_button_release_event (GdkEventButton *ev, MixerStrip *strip)
{
	if (ev->button == 1) {

		/* this allows the user to click on the strip to terminate comment
		   editing. XXX it needs improving so that we don't select the strip
		   at the same time.
		*/
		
		if (_selection.selected (strip->route())) {
			_selection.remove (strip->route());
		} else {
			if (Keyboard::modifier_state_equals (ev->state, Keyboard::TertiaryModifier)) {
				_selection.add (strip->route());
			} else {
				_selection.set (strip->route());
			}
		}
	}

	return true;
}

void
Mixer_UI::connect_to_session (Session* sess)
{

	session = sess;

	XMLNode* node = ARDOUR_UI::instance()->mixer_settings();
	set_state (*node);

	WindowTitle title(session->name());
	title += _("Mixer");
	title += Glib::get_application_name();

	set_title (title.get_string());

	initial_track_display ();

	session->GoingAway.connect (mem_fun(*this, &Mixer_UI::disconnect_from_session));
	session->RouteAdded.connect (mem_fun(*this, &Mixer_UI::add_strip));
	session->mix_group_added.connect (mem_fun(*this, &Mixer_UI::add_mix_group));
	session->mix_group_removed.connect (mem_fun(*this, &Mixer_UI::mix_groups_changed));

	mix_groups_changed ();
	
	_plugin_selector->set_session (session);

	if (_visible) {
	       show_window();
	}

	start_updating ();
}

void
Mixer_UI::disconnect_from_session ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &Mixer_UI::disconnect_from_session));
	
	group_model->clear ();
	_selection.clear ();

	WindowTitle title(Glib::get_application_name());
	title += _("Mixer");
	set_title (title.get_string());
	
	stop_updating ();
}

void
Mixer_UI::show_strip (MixerStrip* ms)
{
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;
	
	for (i = rows.begin(); i != rows.end(); ++i) {
	
		MixerStrip* strip = (*i)[track_columns.strip];
		if (strip == ms) {
			(*i)[track_columns.visible] = true;
			break;
		}
	}
}

void
Mixer_UI::hide_strip (MixerStrip* ms)
{
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;
	
	for (i = rows.begin(); i != rows.end(); ++i) {
		
		MixerStrip* strip = (*i)[track_columns.strip];
		if (strip == ms) {
			(*i)[track_columns.visible] = false;
			break;
		}
	}
}

gint
Mixer_UI::start_updating ()
{
    fast_screen_update_connection = ARDOUR_UI::instance()->SuperRapidScreenUpdate.connect (mem_fun(*this, &Mixer_UI::fast_update_strips));
    return 0;
}

gint
Mixer_UI::stop_updating ()
{
    fast_screen_update_connection.disconnect();
    return 0;
}

void
Mixer_UI::fast_update_strips ()
{
	if (is_mapped () && session) {
		for (list<MixerStrip *>::iterator i = strips.begin(); i != strips.end(); ++i) {
			(*i)->fast_update ();
		}
	}
}

void
Mixer_UI::set_all_strips_visibility (bool yn)
{
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;

	no_track_list_redisplay = true;

	for (i = rows.begin(); i != rows.end(); ++i) {

		TreeModel::Row row = (*i);
		MixerStrip* strip = row[track_columns.strip];
		
		if (strip == 0) {
			continue;
		}
		
		if (strip->route()->is_master() || strip->route()->is_control()) {
			continue;
		}

		(*i)[track_columns.visible] = yn;
	}

	no_track_list_redisplay = false;
	redisplay_track_list ();
}


void
Mixer_UI::set_all_audio_visibility (int tracks, bool yn) 
{
        TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;

	no_track_list_redisplay = true;

	for (i = rows.begin(); i != rows.end(); ++i) {
		TreeModel::Row row = (*i);
		MixerStrip* strip = row[track_columns.strip];

		if (strip == 0) {
			continue;
		}

		if (strip->route()->is_master() || strip->route()->is_control()) {
			continue;
		}

		boost::shared_ptr<AudioTrack> at = strip->audio_track();

		switch (tracks) {
		case 0:
			(*i)[track_columns.visible] = yn;
			break;
			
		case 1:
			if (at) { /* track */
				(*i)[track_columns.visible] = yn;
			}
			break;
			
		case 2:
			if (!at) { /* bus */
				(*i)[track_columns.visible] = yn;
			}
			break;
		}
	}

	no_track_list_redisplay = false;
	redisplay_track_list ();
}

void
Mixer_UI::hide_all_routes ()
{
	set_all_strips_visibility (false);
}

void
Mixer_UI::show_all_routes ()
{
	set_all_strips_visibility (true);
}

void
Mixer_UI::show_all_audiobus ()
{
	set_all_audio_visibility (2, true);
}
void
Mixer_UI::hide_all_audiobus ()
{
	set_all_audio_visibility (2, false);
}

void
Mixer_UI::show_all_audiotracks()
{
	set_all_audio_visibility (1, true);
}
void
Mixer_UI::hide_all_audiotracks ()
{
	set_all_audio_visibility (1, false);
}

void
Mixer_UI::track_list_reorder (const TreeModel::Path& path, const TreeModel::iterator& iter, int* new_order)
{
	session->set_remote_control_ids();
	redisplay_track_list ();
}

void
Mixer_UI::track_list_change (const Gtk::TreeModel::Path& path,const Gtk::TreeModel::iterator& iter)
{
	session->set_remote_control_ids();
	redisplay_track_list ();
}

void
Mixer_UI::track_list_delete (const Gtk::TreeModel::Path& path)
{
	session->set_remote_control_ids();
	ignore_route_reorder = true;
	redisplay_track_list ();
	ignore_route_reorder = false;
}

void
Mixer_UI::redisplay_track_list ()
{
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;
	long order;

	if (no_track_list_redisplay) {
		return;
	}

	for (order = 0, i = rows.begin(); i != rows.end(); ++i, ++order) {
		MixerStrip* strip = (*i)[track_columns.strip];

		if (strip == 0) {
			/* we're in the middle of changing a row, don't worry */
			continue;
		}

		bool visible = (*i)[track_columns.visible];

		if (visible) {
			strip->set_marked_for_display (true);
			strip->route()->set_order_key (N_("signal"), order);

			if (!ignore_route_reorder) {
				strip->route()->set_order_key (get_order_key(), order);
			} 

			if (strip->packed()) {

				if (strip->route()->is_master() || strip->route()->is_control()) {
					out_packer.reorder_child (*strip, -1);
				} else {
					strip_packer.reorder_child (*strip, -1); /* put at end */
				}

			} else {

				if (strip->route()->is_master() || strip->route()->is_control()) {
					out_packer.pack_start (*strip, false, false);
				} else {
					strip_packer.pack_start (*strip, false, false);
				}
				strip->set_packed (true);
				//strip->show();
			}

		} else {

			if (strip->route()->is_master() || strip->route()->is_control()) {
				/* do nothing, these cannot be hidden */
			} else {
				if (strip->packed()) {
					strip_packer.remove (*strip);
					strip->set_packed (false);
				}
			}
		}
	}
	
	if (Config->get_sync_all_route_ordering() && !ignore_route_reorder) {
		ignore_sync = true;
		Route::SyncOrderKeys (); // EMIT SIGNAL
		ignore_sync = false;
	}

	// Rebind all of the midi controls automatically
	
	if (auto_rebinding)
		auto_rebind_midi_controls ();

}

void
Mixer_UI::set_auto_rebinding( bool val )
{
	if( val == TRUE )
	{
		auto_rebinding = TRUE;
		Session::AutoBindingOff();
	}
	else
	{
		auto_rebinding = FALSE;
		Session::AutoBindingOn();
	}
}

void 
Mixer_UI::toggle_auto_rebinding() 
{
	if (auto_rebinding)
	{
		set_auto_rebinding( FALSE );
	}
	
	else
	{
		set_auto_rebinding( TRUE );
	}

	auto_rebind_midi_controls();
}

void 
Mixer_UI::auto_rebind_midi_controls () 
{
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;
	int pos;

	// Create bindings for all visible strips and remove those that are not visible
	pos = 1;  // 0 is reserved for the master strip
	for (i = rows.begin(); i != rows.end(); ++i) {
		MixerStrip* strip = (*i)[track_columns.strip];
    
		if ( (*i)[track_columns.visible] == true ) {  // add bindings for
			// make the actual binding
			//cout<<"Auto Binding:  Visible Strip Found: "<<strip->name()<<endl;

			int controlValue = pos;
			if( strip->route()->is_master() ) {
				controlValue = 0;
			}
			else {
				pos++;
			}

			PBD::Controllable::CreateBinding ( strip->solo_button->get_controllable().get(), controlValue, 0);
			PBD::Controllable::CreateBinding ( strip->mute_button->get_controllable().get(), controlValue, 1);

			if( strip->is_audio_track() ) {
				PBD::Controllable::CreateBinding ( strip->rec_enable_button->get_controllable().get(), controlValue, 2);
			}

			PBD::Controllable::CreateBinding ( strip->gpm.get_controllable().get(), controlValue, 3);
			PBD::Controllable::CreateBinding ( strip->panners.get_controllable().get(), controlValue, 4);

		}
		else {  // Remove any existing binding
			PBD::Controllable::DeleteBinding ( strip->solo_button->get_controllable().get() );
			PBD::Controllable::DeleteBinding ( strip->mute_button->get_controllable().get() );

			if( strip->is_audio_track() ) {
				PBD::Controllable::DeleteBinding ( strip->rec_enable_button->get_controllable().get() );
			}

			PBD::Controllable::DeleteBinding ( strip->gpm.get_controllable().get() );
			PBD::Controllable::DeleteBinding ( strip->panners.get_controllable().get() ); // This only takes the first panner if there are multiples...
		}

	} // for
  
}

struct SignalOrderRouteSorter {
    bool operator() (boost::shared_ptr<Route> a, boost::shared_ptr<Route> b) {
	    /* use of ">" forces the correct sort order */
	    return a->order_key (Mixer_UI::get_order_key()) < b->order_key (Mixer_UI::get_order_key());
    }
};

void
Mixer_UI::initial_track_display ()
{
	boost::shared_ptr<Session::RouteList> routes = session->get_routes();
	Session::RouteList copy (*routes);
	SignalOrderRouteSorter sorter;

	copy.sort (sorter);
	
	no_track_list_redisplay = true;

	track_model->clear ();

	add_strip (copy);

	no_track_list_redisplay = false;

	redisplay_track_list ();
}

void
Mixer_UI::show_track_list_menu ()
{
	if (track_menu == 0) {
		build_track_menu ();
	}

	track_menu->popup (1, gtk_get_current_event_time());
}

bool
Mixer_UI::track_display_button_press (GdkEventButton* ev)
{
	if (Keyboard::is_context_menu_event (ev)) {
		show_track_list_menu ();
		return true;
	}

	TreeIter iter;
	TreeModel::Path path;
	TreeViewColumn* column;
	int cellx;
	int celly;
	
	if (!track_display.get_path_at_pos ((int)ev->x, (int)ev->y, path, column, cellx, celly)) {
		return false;
	}

	switch (GPOINTER_TO_UINT (column->get_data (X_("colnum")))) {
	case 0:
		/* allow normal processing to occur */
		return false;

	case 1: /* visibility */

		if ((iter = track_model->get_iter (path))) {
			MixerStrip* strip = (*iter)[track_columns.strip];
			if (strip) {

				if (!strip->route()->is_master() && !strip->route()->is_control()) {
					bool visible = (*iter)[track_columns.visible];
					(*iter)[track_columns.visible] = !visible;
				}
			}
		}
		return true;

	default:
		break;
	}

	return false;
}


void
Mixer_UI::build_track_menu ()
{
	using namespace Menu_Helpers;
	using namespace Gtk;

	track_menu = new Menu;
	track_menu->set_name ("ArdourContextMenu");
	MenuList& items = track_menu->items();
	
	items.push_back (MenuElem (_("Show All"), mem_fun(*this, &Mixer_UI::show_all_routes)));
	items.push_back (MenuElem (_("Hide All"), mem_fun(*this, &Mixer_UI::hide_all_routes)));
	items.push_back (MenuElem (_("Show All Audio Tracks"), mem_fun(*this, &Mixer_UI::show_all_audiotracks)));
	items.push_back (MenuElem (_("Hide All Audio Tracks"), mem_fun(*this, &Mixer_UI::hide_all_audiotracks)));
	items.push_back (MenuElem (_("Show All Audio Busses"), mem_fun(*this, &Mixer_UI::show_all_audiobus)));
	items.push_back (MenuElem (_("Hide All Audio Busses"), mem_fun(*this, &Mixer_UI::hide_all_audiobus)));

}

void
Mixer_UI::strip_name_changed (MixerStrip* mx)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Mixer_UI::strip_name_changed), mx));
	
	TreeModel::Children rows = track_model->children();
	TreeModel::Children::iterator i;
	
	for (i = rows.begin(); i != rows.end(); ++i) {
		if ((*i)[track_columns.strip] == mx) {
			(*i)[track_columns.text] = mx->route()->name();
			return;
		}
	} 

	error << _("track display list item for renamed strip not found!") << endmsg;
}


void
Mixer_UI::build_mix_group_context_menu ()
{
	using namespace Gtk::Menu_Helpers;

	mix_group_context_menu = new Menu;
	mix_group_context_menu->set_name ("ArdourContextMenu");
	MenuList& items = mix_group_context_menu->items();

	items.push_back (MenuElem (_("Activate All"), mem_fun(*this, &Mixer_UI::activate_all_mix_groups)));
	items.push_back (MenuElem (_("Disable All"), mem_fun(*this, &Mixer_UI::disable_all_mix_groups)));
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Add group"), mem_fun(*this, &Mixer_UI::new_mix_group)));
	
}

bool
Mixer_UI::group_display_button_press (GdkEventButton* ev)
{
	if (Keyboard::is_context_menu_event (ev)) {
		if (mix_group_context_menu == 0) {
			build_mix_group_context_menu ();
		}
		mix_group_context_menu->popup (1, ev->time);
		return true;
	}


        RouteGroup* group;
	TreeIter iter;
	TreeModel::Path path;
	TreeViewColumn* column;
	int cellx;
	int celly;

	if (!group_display.get_path_at_pos ((int)ev->x, (int)ev->y, path, column, cellx, celly)) {
		return false;
	}

	switch (GPOINTER_TO_UINT (column->get_data (X_("colnum")))) {
	case 0:
		if (Keyboard::is_edit_event (ev)) {
			if ((iter = group_model->get_iter (path))) {
				if ((group = (*iter)[group_columns.group]) != 0) {
					// edit_mix_group (group);
					return true;
				}
			}
			
		} 
		break;

	case 1:
		if ((iter = group_model->get_iter (path))) {
			bool active = (*iter)[group_columns.active];
			(*iter)[group_columns.active] = !active;
			return true;
		}
		break;
		
	case 2:
		if ((iter = group_model->get_iter (path))) {
			bool visible = (*iter)[group_columns.visible];
			(*iter)[group_columns.visible] = !visible;
			return true;
		}
		break;

	default:
		break;
	}
	
	return false;
 }

void
Mixer_UI::activate_all_mix_groups ()
{
        Gtk::TreeModel::Children children = group_model->children();
	for(Gtk::TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter) {
	        (*iter)[group_columns.active] = true;
	}
}

void
Mixer_UI::disable_all_mix_groups ()
{
        Gtk::TreeModel::Children children = group_model->children();
	for(Gtk::TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter) {
	        (*iter)[group_columns.active] = false;
	}
}

void
Mixer_UI::mix_groups_changed ()
{
	ENSURE_GUI_THREAD (mem_fun (*this, &Mixer_UI::mix_groups_changed));

	/* just rebuild the while thing */

	group_model->clear ();

	{
		TreeModel::Row row;
		row = *(group_model->append());
		row[group_columns.active] = false;
		row[group_columns.visible] = true;
		row[group_columns.text] = (_("-all-"));
		row[group_columns.group] = 0;
	}

	session->foreach_mix_group (mem_fun (*this, &Mixer_UI::add_mix_group));
}

void
Mixer_UI::new_mix_group ()
{
	session->add_mix_group ("");
}

void
Mixer_UI::remove_selected_mix_group ()
{
	Glib::RefPtr<TreeSelection> selection = group_display.get_selection();
	TreeView::Selection::ListHandle_Path rows = selection->get_selected_rows ();

	if (rows.empty()) {
		return;
	}

	TreeView::Selection::ListHandle_Path::iterator i = rows.begin();
	TreeIter iter;
	
	/* selection mode is single, so rows.begin() is it */

	if ((iter = group_model->get_iter (*i))) {

		RouteGroup* rg = (*iter)[group_columns.group];

		if (rg) {
			session->remove_mix_group (*rg);
		}
	}
}

void
Mixer_UI::group_flags_changed (void* src, RouteGroup* group)
{
	if (in_group_row_change) {
		return;
	}

	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Mixer_UI::group_flags_changed), src, group));

	/* force an update of any mixer strips that are using this group,
	   otherwise mix group names don't change in mixer strips 
	*/

	for (list<MixerStrip *>::iterator i = strips.begin(); i != strips.end(); ++i) {
		if ((*i)->mix_group() == group) {
			(*i)->mix_group_changed(0);
		}
	}
	
	TreeModel::iterator i;
	TreeModel::Children rows = group_model->children();
	Glib::RefPtr<TreeSelection> selection = group_display.get_selection();

	in_group_row_change = true;
	
	for (i = rows.begin(); i != rows.end(); ++i) {
		if ((*i)[group_columns.group] == group) {
			(*i)[group_columns.visible] = !group->is_hidden ();
			(*i)[group_columns.active] = group->is_active ();
			(*i)[group_columns.text] = group->name ();
			break;
		}
	}

	in_group_row_change = false;
}

void
Mixer_UI::mix_group_name_edit (const Glib::ustring& path, const Glib::ustring& new_text)
{
	RouteGroup* group;
	TreeIter iter;

	if ((iter = group_model->get_iter (path))) {
	
		if ((group = (*iter)[group_columns.group]) == 0) {
			return;
		}
		
		if (new_text != group->name()) {
			group->set_name (new_text);
		}
	}
}

void 
Mixer_UI::mix_group_row_change (const Gtk::TreeModel::Path& path,const Gtk::TreeModel::iterator& iter)
{
	RouteGroup* group;

	if (in_group_row_change) {
		return;
	}

	if ((group = (*iter)[group_columns.group]) == 0) {
		return;
	}

	if ((*iter)[group_columns.visible]) {
		for (list<MixerStrip *>::iterator i = strips.begin(); i != strips.end(); ++i) {
			if ((*i)->mix_group() == group) {
				show_strip (*i);
			}
		}
	} else {
		for (list<MixerStrip *>::iterator i = strips.begin(); i != strips.end(); ++i) {
			if ((*i)->mix_group() == group) {
				hide_strip (*i);
			}
		}
	} 

	bool active = (*iter)[group_columns.active];
	group->set_active (active, this);

	Glib::ustring name = (*iter)[group_columns.text];

	if (name != group->name()) {
		group->set_name (name);
	}

}

void
Mixer_UI::add_mix_group (RouteGroup* group)

{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &Mixer_UI::add_mix_group), group));
	bool focus = false;

	in_group_row_change = true;

	TreeModel::Row row = *(group_model->append());
	row[group_columns.active] = group->is_active();
	row[group_columns.visible] = true;
	row[group_columns.group] = group;
	if (!group->name().empty()) {
		row[group_columns.text] = group->name();
	} else {
		row[group_columns.text] = _("unnamed");
		focus = true;
	}

	group->FlagsChanged.connect (bind (mem_fun(*this, &Mixer_UI::group_flags_changed), group));
	
	if (focus) {
		TreeViewColumn* col = group_display.get_column (0);
		CellRendererText* name_cell = dynamic_cast<CellRendererText*>(group_display.get_column_cell_renderer (0));
		group_display.set_cursor (group_model->get_path (row), *col, *name_cell, true);
	}

	in_group_row_change = false;
}

bool
Mixer_UI::strip_scroller_button_release (GdkEventButton* ev)
{
	using namespace Menu_Helpers;

	if (Keyboard::is_context_menu_event (ev)) {
		ARDOUR_UI::instance()->add_route (this);
		return true;
	}

	return false;
}

void
Mixer_UI::set_strip_width (Width w)
{
	_strip_width = w;

	for (list<MixerStrip*>::iterator i = strips.begin(); i != strips.end(); ++i) {
		(*i)->set_width (w, this);
	}
}

void
Mixer_UI::set_window_pos_and_size ()
{
	resize (m_width, m_height);
	move (m_root_x, m_root_y);
}

	void
Mixer_UI::get_window_pos_and_size ()
{
	get_position(m_root_x, m_root_y);
	get_size(m_width, m_height);
}

int
Mixer_UI::set_state (const XMLNode& node)
{
	const XMLProperty* prop;
	XMLNode* geometry;
	
	if ((geometry = find_named_node (node, "geometry")) == 0) {

		m_width = default_width;
		m_height = default_height;
		m_root_x = 1;
		m_root_y = 1;

	} else {

		m_width = atoi(geometry->property("x_size")->value().c_str());
		m_height = atoi(geometry->property("y_size")->value().c_str());
		m_root_x = atoi(geometry->property("x_pos")->value().c_str());
		m_root_y = atoi(geometry->property("y_pos")->value().c_str());
	}

	set_window_pos_and_size ();

	if ((prop = node.property ("narrow-strips"))) {
		if (prop->value() == "yes") {
			set_strip_width (Narrow);
		} else {
			set_strip_width (Wide);
		}
	}

	if ((prop = node.property ("show-mixer"))) {
		if (prop->value() == "yes") {
		       _visible = true;
		}
	}

	return 0;
}

XMLNode&
Mixer_UI::get_state (void)
{
	XMLNode* node = new XMLNode ("Mixer");

	if (is_realized()) {
		Glib::RefPtr<Gdk::Window> win = get_window();
	
		get_window_pos_and_size ();

		XMLNode* geometry = new XMLNode ("geometry");
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", m_width);
		geometry->add_property(X_("x_size"), string(buf));
		snprintf(buf, sizeof(buf), "%d", m_height);
		geometry->add_property(X_("y_size"), string(buf));
		snprintf(buf, sizeof(buf), "%d", m_root_x);
		geometry->add_property(X_("x_pos"), string(buf));
		snprintf(buf, sizeof(buf), "%d", m_root_y);
		geometry->add_property(X_("y_pos"), string(buf));
		
		// written only for compatibility, they are not used.
		snprintf(buf, sizeof(buf), "%d", 0);
		geometry->add_property(X_("x_off"), string(buf));
		snprintf(buf, sizeof(buf), "%d", 0);
		geometry->add_property(X_("y_off"), string(buf));

		snprintf(buf,sizeof(buf), "%d",gtk_paned_get_position (static_cast<Paned*>(&rhs_pane1)->gobj()));
		geometry->add_property(X_("mixer_rhs_pane1_pos"), string(buf));
		snprintf(buf,sizeof(buf), "%d",gtk_paned_get_position (static_cast<Paned*>(&list_hpane)->gobj()));
		geometry->add_property(X_("mixer_list_hpane_pos"), string(buf));

		node->add_child_nocopy (*geometry);
	}

	node->add_property ("narrow-strips", _strip_width == Narrow ? "yes" : "no");

	node->add_property ("show-mixer", _visible ? "yes" : "no");

	return *node;
}


void 
Mixer_UI::pane_allocation_handler (Allocation& alloc, Gtk::Paned* which)
{
	int pos;
	XMLProperty* prop = 0;
	char buf[32];
	XMLNode* node = ARDOUR_UI::instance()->mixer_settings();
	XMLNode* geometry;
	int width, height;
	static int32_t done[3] = { 0, 0, 0 };

	if ((geometry = find_named_node (*node, "geometry")) == 0) {
		width = default_width;
		height = default_height;
	} else {
		width = atoi(geometry->property("x_size")->value());
		height = atoi(geometry->property("y_size")->value());
	}

	if (which == static_cast<Gtk::Paned*> (&rhs_pane1)) {

		if (done[0]) {
			return;
		}

		if (!geometry || (prop = geometry->property("mixer_rhs_pane1_pos")) == 0) {
			pos = height / 3;
			snprintf (buf, sizeof(buf), "%d", pos);
		} else {
			pos = atoi (prop->value());
		}

		if ((done[0] = GTK_WIDGET(rhs_pane1.gobj())->allocation.height > pos)) {
			rhs_pane1.set_position (pos);
		}

	} else if (which == static_cast<Gtk::Paned*> (&list_hpane)) {

		if (done[2]) {
			return;
		}

		if (!geometry || (prop = geometry->property("mixer_list_hpane_pos")) == 0) {
			pos = 75;
			snprintf (buf, sizeof(buf), "%d", pos);
		} else {
			pos = atoi (prop->value());
		}

		if ((done[2] = GTK_WIDGET(list_hpane.gobj())->allocation.width > pos)) {
			list_hpane.set_position (pos);
		}
	}
}

bool
Mixer_UI::on_key_press_event (GdkEventKey* ev)
{
	return key_press_focus_accelerator_handler (*this, ev);
}
