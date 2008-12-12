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

#ifndef __ardour_keyboard_h__
#define __ardour_keyboard_h__

#include <map>
#include <vector>
#include <string>

#include <sigc++/signal.h>
#include <gtk/gtk.h>
#include <gtkmm/accelkey.h>

#include <ardour/types.h>
#include <pbd/stateful.h>

#include "selection.h"

using std::string;

class Keyboard : public sigc::trackable, PBD::Stateful
{
  public:
	Keyboard ();
	~Keyboard ();

	XMLNode& get_state (void);
	int set_state (const XMLNode&);

	typedef std::vector<uint32_t> State;
	typedef uint32_t ModifierMask;

	static uint32_t PrimaryModifier;
	static uint32_t SecondaryModifier;
	static uint32_t TertiaryModifier;
	static uint32_t Level4Modifier;
	static uint32_t CopyModifier;
	static uint32_t RangeSelectModifier;

	static void set_primary_modifier (uint32_t newval) {
		set_modifier (newval, PrimaryModifier);
	}
	static void set_secondary_modifier (uint32_t newval) {
		set_modifier (newval, SecondaryModifier);
	}
	static void set_tertiary_modifier (uint32_t newval) {
		set_modifier (newval, TertiaryModifier);
	}
	static void set_level4_modifier (uint32_t newval) {
		set_modifier (newval, Level4Modifier);
	}
	static void set_copy_modifier (uint32_t newval) {
		set_modifier (newval, CopyModifier);
	}
	static void set_range_select_modifier (uint32_t newval) {
		set_modifier (newval, RangeSelectModifier);
	}

	bool key_is_down (uint32_t keyval);

	static GdkModifierType RelevantModifierKeyMask;

	static bool no_modifier_keys_pressed(GdkEventButton* ev) {
		return (ev->state & RelevantModifierKeyMask) == 0;
	}

	bool leave_window (GdkEventCrossing *ev, Gtk::Window*);
	bool enter_window (GdkEventCrossing *ev, Gtk::Window*);

	static bool modifier_state_contains (guint state, ModifierMask);
	static bool modifier_state_equals   (guint state, ModifierMask);

	static Selection::Operation selection_type (guint state);

	static bool no_modifiers_active (guint state);

	static void set_snap_modifier (guint);
	static ModifierMask snap_modifier () { return ModifierMask (snap_mod); }

	static guint edit_button() { return edit_but; }
	static void set_edit_button (guint);
	static guint edit_modifier() { return edit_mod; }
	static void set_edit_modifier(guint);

	static guint delete_button() { return delete_but; }
	static void set_delete_button(guint);
	static guint delete_modifier() { return delete_mod; }
	static void set_delete_modifier(guint);

	static bool is_edit_event (GdkEventButton*);
	static bool is_delete_event (GdkEventButton*);
	static bool is_context_menu_event (GdkEventButton*);
	static bool is_button2_event (GdkEventButton*);

	static Keyboard& the_keyboard() { return *_the_keyboard; }

	static bool some_magic_widget_has_focus ();
	static void magic_widget_grab_focus ();
	static void magic_widget_drop_focus ();

	static void setup_keybindings ();
	static void keybindings_changed ();
	static void save_keybindings ();
	static bool load_keybindings (std::string path);
	static void set_can_save_keybindings (bool yn);
	static std::string current_binding_name () { return _current_binding_name; }
	static std::map<std::string,std::string> binding_files;

	struct AccelKeyLess {
	    bool operator() (const Gtk::AccelKey a, const Gtk::AccelKey b) const {
		    if (a.get_key() != b.get_key()) {
			    return a.get_key() < b.get_key();
		    } else {
			    return a.get_mod() < b.get_mod();
		    }
	    }
	};

  private:
	static Keyboard* _the_keyboard;

	guint           snooper_id;
	State           state;

	static guint     edit_but;
	static guint     edit_mod;
	static guint     delete_but;
	static guint     delete_mod;
	static guint     snap_mod;
	static guint     button2_modifiers;
	static Gtk::Window* current_window;
	static std::string user_keybindings_path;
	static bool can_save_keybindings;
	static bool bindings_changed_after_save_became_legal;
	static std::string _current_binding_name;

	typedef std::pair<std::string,std::string> two_strings;

	static std::map<Gtk::AccelKey,two_strings,AccelKeyLess> release_keys;

	static gint _snooper (GtkWidget*, GdkEventKey*, gpointer);
	gint snooper (GtkWidget*, GdkEventKey*);

	static void set_modifier (uint32_t newval, uint32_t& variable);

	static bool _some_magic_widget_has_focus;
};

#endif /* __ardour_keyboard_h__ */
