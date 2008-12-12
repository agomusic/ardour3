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

#include <vector>
#include <ardour/ardour.h>

#include "ardour_ui.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <ctype.h>

#include <gtkmm/accelmap.h>

#include <gdk/gdkkeysyms.h>
#include <pbd/error.h>
#include <pbd/file_utils.h>

#include <ardour/filesystem_paths.h>

#include "keyboard.h"
#include "gui_thread.h"
#include "opts.h"
#include "actions.h"

#include "i18n.h"

using namespace PBD;
using namespace ARDOUR;
using namespace Gtk;
using namespace std;

#define KBD_DEBUG 1
bool debug_keyboard = false;

guint Keyboard::edit_but = 3;
guint Keyboard::edit_mod = GDK_CONTROL_MASK;
guint Keyboard::delete_but = 3;
guint Keyboard::delete_mod = GDK_SHIFT_MASK;
guint Keyboard::snap_mod = GDK_MOD3_MASK;

#ifdef GTKOSX
guint Keyboard::PrimaryModifier = GDK_META_MASK;   // Command
guint Keyboard::SecondaryModifier = GDK_MOD1_MASK; // Alt/Option
guint Keyboard::TertiaryModifier = GDK_SHIFT_MASK; // Shift
guint Keyboard::Level4Modifier = GDK_CONTROL_MASK; // Control
guint Keyboard::CopyModifier = GDK_MOD1_MASK;      // Alt/Option
guint Keyboard::RangeSelectModifier = GDK_SHIFT_MASK;   
guint Keyboard::button2_modifiers = Keyboard::SecondaryModifier|Keyboard::Level4Modifier;
#else
guint Keyboard::PrimaryModifier = GDK_CONTROL_MASK; // Control
guint Keyboard::SecondaryModifier = GDK_MOD1_MASK;  // Alt/Option
guint Keyboard::TertiaryModifier = GDK_SHIFT_MASK;  // Shift
guint Keyboard::Level4Modifier = GDK_MOD4_MASK;     // Mod4/Windows
guint Keyboard::CopyModifier = GDK_CONTROL_MASK;    
guint Keyboard::RangeSelectModifier = GDK_SHIFT_MASK;   
guint Keyboard::button2_modifiers = 0; /* not used */
#endif


Keyboard*    Keyboard::_the_keyboard = 0;
Gtk::Window* Keyboard::current_window = 0;
bool         Keyboard::_some_magic_widget_has_focus = false;

std::string Keyboard::user_keybindings_path;
bool Keyboard::can_save_keybindings = false;
bool Keyboard::bindings_changed_after_save_became_legal = false;
map<string,string> Keyboard::binding_files;
string Keyboard::_current_binding_name = _("Unknown");
map<AccelKey,pair<string,string>,Keyboard::AccelKeyLess> Keyboard::release_keys;

/* set this to initially contain the modifiers we care about, then track changes in ::set_edit_modifier() etc. */

GdkModifierType Keyboard::RelevantModifierKeyMask;

void
Keyboard::magic_widget_grab_focus () 
{
	_some_magic_widget_has_focus = true;
}

void
Keyboard::magic_widget_drop_focus ()
{
	_some_magic_widget_has_focus = false;
}

bool
Keyboard::some_magic_widget_has_focus ()
{
	return _some_magic_widget_has_focus;
}

Keyboard::Keyboard ()
{
	if (_the_keyboard == 0) {
		_the_keyboard = this;
	}

	RelevantModifierKeyMask = (GdkModifierType) gtk_accelerator_get_default_mod_mask ();

	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | PrimaryModifier);
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | SecondaryModifier);
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | TertiaryModifier);
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | Level4Modifier);
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | CopyModifier);
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | RangeSelectModifier);

	gtk_accelerator_set_default_mod_mask (RelevantModifierKeyMask);

	snooper_id = gtk_key_snooper_install (_snooper, (gpointer) this);

	XMLNode* node = ARDOUR_UI::instance()->keyboard_settings();
	set_state (*node);
}

Keyboard::~Keyboard ()
{
	gtk_key_snooper_remove (snooper_id);
}

XMLNode& 
Keyboard::get_state (void)
{
	XMLNode* node = new XMLNode ("Keyboard");
	char buf[32];

	snprintf (buf, sizeof (buf), "%d", edit_but);
	node->add_property ("edit-button", buf);
	snprintf (buf, sizeof (buf), "%d", edit_mod);
	node->add_property ("edit-modifier", buf);
	snprintf (buf, sizeof (buf), "%d", delete_but);
	node->add_property ("delete-button", buf);
	snprintf (buf, sizeof (buf), "%d", delete_mod);
	node->add_property ("delete-modifier", buf);
	snprintf (buf, sizeof (buf), "%d", snap_mod);
	node->add_property ("snap-modifier", buf);

	return *node;
}

int 
Keyboard::set_state (const XMLNode& node)
{
	const XMLProperty* prop;

	if ((prop = node.property ("edit-button")) != 0) {
		sscanf (prop->value().c_str(), "%d", &edit_but);
	} 

	if ((prop = node.property ("edit-modifier")) != 0) {
		sscanf (prop->value().c_str(), "%d", &edit_mod);
	} 

	if ((prop = node.property ("delete-button")) != 0) {
		sscanf (prop->value().c_str(), "%d", &delete_but);
	} 

	if ((prop = node.property ("delete-modifier")) != 0) {
		sscanf (prop->value().c_str(), "%d", &delete_mod);
	} 

	if ((prop = node.property ("snap-modifier")) != 0) {
		sscanf (prop->value().c_str(), "%d", &snap_mod);
	} 

	return 0;
}

gint
Keyboard::_snooper (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	return ((Keyboard *) data)->snooper (widget, event);
}

gint
Keyboard::snooper (GtkWidget *widget, GdkEventKey *event)
{
	uint32_t keyval;
	bool ret = false;

#if 0
	cerr << "snoop widget " << widget << " key " << event->keyval << " type: " << event->type 
	     << " state " << std::hex << event->state << std::dec
	     << endl;
#endif

#if KBD_DEBUG
	if (debug_keyboard) {
		cerr << "snoop widget " << widget << " key " << event->keyval << " type: " << event->type 
		     << endl;
	}
#endif

	if (event->keyval == GDK_Shift_R) {
		keyval = GDK_Shift_L;

	} else 	if (event->keyval == GDK_Control_R) {
		keyval = GDK_Control_L;

	} else {
		keyval = event->keyval;
	}
		
	if (event->type == GDK_KEY_PRESS) {

		if (find (state.begin(), state.end(), keyval) == state.end()) {
			state.push_back (keyval);
			sort (state.begin(), state.end());

		} else {

			/* key is already down. if its also used for release,
			   prevent auto-repeat events.
			*/

			for (map<AccelKey,two_strings,AccelKeyLess>::iterator k = release_keys.begin(); k != release_keys.end(); ++k) {

				const AccelKey& ak (k->first);
				
				if (keyval == ak.get_key() && (Gdk::ModifierType)(event->state | Gdk::RELEASE_MASK) == ak.get_mod()) {
					ret = true;
					break;
				}
			}
		}

	} else if (event->type == GDK_KEY_RELEASE) {

		State::iterator i;
		
		if ((i = find (state.begin(), state.end(), keyval)) != state.end()) {
			state.erase (i);
			sort (state.begin(), state.end());
		} 

		for (map<AccelKey,two_strings,AccelKeyLess>::iterator k = release_keys.begin(); k != release_keys.end(); ++k) {

			const AccelKey& ak (k->first);
			two_strings ts (k->second);

			if (keyval == ak.get_key() && (Gdk::ModifierType)(event->state | Gdk::RELEASE_MASK) == ak.get_mod()) {
				Glib::RefPtr<Gtk::Action> act = ActionManager::get_action (ts.first.c_str(), ts.second.c_str());
				if (act) {
					act->activate();
					ret = true;
				}
				break;
			}
		}
	}

	/* Special keys that we want to handle in
	   any dialog, no matter whether it uses
	   the regular set of accelerators or not
	*/

	if (event->type == GDK_KEY_RELEASE && modifier_state_equals (event->state, PrimaryModifier)) {
		switch (event->keyval) {
		case GDK_w:
			if (current_window) {
				current_window->hide ();
				current_window = 0;
				ret = true;
			}
			break;
		}
	}

	return ret;
}

bool
Keyboard::key_is_down (uint32_t keyval)
{
	return find (state.begin(), state.end(), keyval) != state.end();
}

bool
Keyboard::enter_window (GdkEventCrossing *ev, Gtk::Window* win)
{
	current_window = win;
	return false;
}

bool
Keyboard::leave_window (GdkEventCrossing *ev, Gtk::Window* win)
{
	if (ev) {
		switch (ev->detail) {
		case GDK_NOTIFY_INFERIOR:
			if (debug_keyboard) {
				cerr << "INFERIOR crossing ... out\n";
			}
			break;
			
		case GDK_NOTIFY_VIRTUAL:
			if (debug_keyboard) {
				cerr << "VIRTUAL crossing ... out\n";
			}
			/* fallthru */
			
		default:
			if (debug_keyboard) {
				cerr << "REAL CROSSING ... out\n";
				cerr << "clearing current target\n";
			}
			state.clear ();
			current_window = 0;
		}
	} else {
		current_window = 0;
	}

	return false;
}

void
Keyboard::set_edit_button (guint but)
{
	edit_but = but;
}

void
Keyboard::set_edit_modifier (guint mod)
{
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask & ~edit_mod);
	edit_mod = mod;
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | edit_mod);
}

void
Keyboard::set_delete_button (guint but)
{
	delete_but = but;
}

void
Keyboard::set_delete_modifier (guint mod)
{
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask & ~delete_mod);
	delete_mod = mod;
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | delete_mod);
}

void
Keyboard::set_modifier (uint32_t newval, uint32_t& var)
{
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask & ~var);
	var = newval;
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | var);
}

void
Keyboard::set_snap_modifier (guint mod)
{
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask & ~snap_mod);
	snap_mod = mod;
	RelevantModifierKeyMask = GdkModifierType (RelevantModifierKeyMask | snap_mod);
}

bool
Keyboard::is_edit_event (GdkEventButton *ev)
{
	return (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) && 
		(ev->button == Keyboard::edit_button()) && 
		((ev->state & RelevantModifierKeyMask) == Keyboard::edit_modifier());
}

bool
Keyboard::is_button2_event (GdkEventButton* ev)
{
#ifdef GTKOSX
	return (ev->button == 2) || 
		((ev->button == 1) && 
		 ((ev->state & Keyboard::button2_modifiers) == Keyboard::button2_modifiers));
#else
	return ev->button == 2;
#endif	
}

bool
Keyboard::is_delete_event (GdkEventButton *ev)
{
	return (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) && 
		(ev->button == Keyboard::delete_button()) && 
		((ev->state & RelevantModifierKeyMask) == Keyboard::delete_modifier());
}

bool
Keyboard::is_context_menu_event (GdkEventButton *ev)
{
	return (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) && 
		(ev->button == 3) && 
		((ev->state & RelevantModifierKeyMask) == 0);
}

bool 
Keyboard::no_modifiers_active (guint state)
{
	return (state & RelevantModifierKeyMask) == 0;
}

bool
Keyboard::modifier_state_contains (guint state, ModifierMask mask)
{
	return (state & mask) == (guint) mask;
}

bool
Keyboard::modifier_state_equals (guint state, ModifierMask mask)
{
	return (state & RelevantModifierKeyMask) == (guint) mask;
}

Selection::Operation
Keyboard::selection_type (guint state)
{
	/* note that there is no modifier for "Add" */

	if (modifier_state_equals (state, RangeSelectModifier)) {
		return Selection::Extend;
	} else if (modifier_state_equals (state, PrimaryModifier)) {
		return Selection::Toggle;
	} else {
		return Selection::Set;
	}
}


static void 
accel_map_changed (GtkAccelMap* map,
		   gchar* path,
		   guint  key,
		   GdkModifierType mod,
		   gpointer arg)
{
	Keyboard::keybindings_changed ();
}

void
Keyboard::keybindings_changed ()
{
	if (Keyboard::can_save_keybindings) {
		Keyboard::bindings_changed_after_save_became_legal = true;
	}

	Keyboard::save_keybindings ();
}

void
Keyboard::set_can_save_keybindings (bool yn)
{
	can_save_keybindings = yn;
}

void
Keyboard::save_keybindings ()
{
	if (can_save_keybindings && bindings_changed_after_save_became_legal) {
		Gtk::AccelMap::save (user_keybindings_path);
	} 
}

void
Keyboard::setup_keybindings ()
{
	using namespace ARDOUR_COMMAND_LINE;
	std::string default_bindings = "mnemonic-us.bindings";
	vector<string> strs;

	binding_files.clear ();

	ARDOUR::find_bindings_files (binding_files);

	/* set up the per-user bindings path */
	
	strs.push_back (Glib::get_home_dir());
	strs.push_back (".ardour3");
	strs.push_back ("ardour.bindings");

	user_keybindings_path = Glib::build_filename (strs);

	if (Glib::file_test (user_keybindings_path, Glib::FILE_TEST_EXISTS)) {
		std::pair<string,string> newpair;
		newpair.first = _("your own");
		newpair.second = user_keybindings_path;
		binding_files.insert (newpair);
	}

	/* check to see if they gave a style name ("SAE", "ergonomic") or
	   an actual filename (*.bindings)
	*/

	if (!keybindings_path.empty() && keybindings_path.find (".bindings") == string::npos) {
		
		// just a style name - allow user to
		// specify the layout type. 
		
		char* layout;
		
		if ((layout = getenv ("ARDOUR_KEYBOARD_LAYOUT")) != 0 && layout[0] != '\0') {
			
			/* user-specified keyboard layout */
			
			keybindings_path += '-';
			keybindings_path += layout;

		} else {

			/* default to US/ANSI - we have to pick something */

			keybindings_path += "-us";
		}
		
		keybindings_path += ".bindings";
	} 

	if (keybindings_path.empty()) {

		/* no path or binding name given: check the user one first */

		if (!Glib::file_test (user_keybindings_path, Glib::FILE_TEST_EXISTS)) {
			
			keybindings_path = "";

		} else {
			
			keybindings_path = user_keybindings_path;
		}
	} 

	/* if we still don't have a path at this point, use the default */

	if (keybindings_path.empty()) {
		keybindings_path = default_bindings;
	}

	while (true) {

		if (!Glib::path_is_absolute (keybindings_path)) {
			
			/* not absolute - look in the usual places */
			sys::path keybindings_file;

			SearchPath spath = ardour_search_path() + user_config_directory() + system_config_search_path();

			if ( ! find_file_in_search_path (spath, keybindings_path, keybindings_file)) {
				
				if (keybindings_path == default_bindings) {
					error << _("Default keybindings not found - Ardour will be hard to use!") << endmsg;
					return;
				} else {
					warning << string_compose (_("Key bindings file \"%1\" not found. Default bindings used instead"), 
								   keybindings_path)
						<< endmsg;
					keybindings_path = default_bindings;
				}

			} else {

				/* use it */

				keybindings_path = keybindings_file.to_string();
				break;
				
			}

		} else {
			
			/* path is absolute already */

			if (!Glib::file_test (keybindings_path, Glib::FILE_TEST_EXISTS)) {
				if (keybindings_path == default_bindings) {
					error << _("Default keybindings not found - Ardour will be hard to use!") << endmsg;
					return;
				} else {
					warning << string_compose (_("Key bindings file \"%1\" not found. Default bindings used instead"), 
								   keybindings_path)
						<< endmsg;
					keybindings_path = default_bindings;
				}

			} else {
				break;
			}
		}
	}

	load_keybindings (keybindings_path);

	/* catch changes */

	GtkAccelMap* accelmap = gtk_accel_map_get();
	g_signal_connect (accelmap, "changed", (GCallback) accel_map_changed, 0);
}

bool
Keyboard::load_keybindings (string path)
{
	try {
		cerr << "loading bindings from " << path << endl;

		Gtk::AccelMap::load (path);

		_current_binding_name = _("Unknown");

		for (map<string,string>::iterator x = binding_files.begin(); x != binding_files.end(); ++x) {
			if (path == x->second) {
				_current_binding_name = x->first;
				break;
			}
		}


	} catch (...) {
		error << string_compose (_("Ardour key bindings file not found at \"%1\" or contains errors."), path)
		      << endmsg;
		return false;
	}

	/* now find all release-driven bindings */

	vector<string> groups;
	vector<string> names;
	vector<AccelKey> bindings;
	
	ActionManager::get_all_actions (groups, names, bindings);
	
	vector<string>::iterator g;
	vector<AccelKey>::iterator b;
	vector<string>::iterator n;

	release_keys.clear ();

	for (n = names.begin(), b = bindings.begin(), g = groups.begin(); n != names.end(); ++n, ++b, ++g) {
		if ((*b).get_mod() & Gdk::RELEASE_MASK) {
			release_keys.insert (pair<AccelKey,two_strings> (*b, two_strings (*g, *n)));
		}
	}

	return true;
}


