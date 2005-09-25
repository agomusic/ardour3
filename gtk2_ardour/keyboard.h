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

    $Id$
*/

#ifndef __ardour_keyboard_h__
#define __ardour_keyboard_h__

#include <vector>
#include <string>

#include <sigc++/signal_system.h>
#include <gtk/gtk.h>

#include <ardour/types.h>
#include <ardour/stateful.h>

using std::vector;
using std::string;

class KeyboardTarget;
class ArdourDialog;

class Keyboard : public SigC::Object, Stateful
{
  public:
	Keyboard ();
	~Keyboard ();

	XMLNode& get_state (void);
	int set_state (const XMLNode&);

	typedef vector<uint32_t> State;
	
	void set_target (KeyboardTarget *);
	void set_default_target (KeyboardTarget *);
	void allow_focus (bool);

	gint focus_in_handler (GdkEventFocus*);
	gint focus_out_handler (GdkEventFocus*);

	int  get_prefix(float&, bool& was_floating);
	void start_prefix ();

	static State  translate_key_name (const string&);
	static string get_real_keyname (const string& name);

	void register_target (KeyboardTarget *);

	void set_current_dialog (ArdourDialog*);
	void close_current_dialog ();

	typedef uint32_t ModifierMask;

	static uint32_t Control;
	static uint32_t Shift;
	static uint32_t Alt;
	static uint32_t Meta;

	bool key_is_down (uint32_t keyval);

	static GdkModifierType RelevantModifierKeyMask;

	static bool no_modifier_keys_pressed(GdkEventButton* ev) {
		return (ev->state & RelevantModifierKeyMask) == 0;
	}

	static bool modifier_state_contains (guint state, ModifierMask);
	static bool modifier_state_equals   (guint state, ModifierMask);

	static bool no_modifiers_active (guint state);

	static void set_meta_modifier (guint);

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

	static Keyboard& the_keyboard() { return *_the_keyboard; }

  private:
	static Keyboard* _the_keyboard;

	bool   _queue_events;
	bool   _flush_queue;
	guint32 playback_ignore_count;

	guint           snooper_id;
	State           state;
	KeyboardTarget* target;
	KeyboardTarget* default_target;
	bool            focus_allowed;
	bool            collecting_prefix;
	string          current_prefix;
	int*            modifier_masks;
	int             modifier_mask;
	int             min_keycode;
	int             max_keycode;
	ArdourDialog*   current_dialog;
	std::vector<ArdourDialog*> known_dialogs;

	static guint     edit_but;
	static guint     edit_mod;
	static guint     delete_but;
	static guint     delete_mod;
	static guint     snap_mod;

	static gint _snooper (GtkWidget*, GdkEventKey*, gpointer);
	gint snooper (GtkWidget*, GdkEventKey*);
	
	void maybe_unset_target (KeyboardTarget *);
	void queue_event (GdkEventKey*);
	void playback_queue ();
	void clear_queue ();
	void get_modifier_masks ();
	void check_modifier_state ();
	void clear_modifier_state ();
	gint enter_window (GdkEventCrossing*, KeyboardTarget*);
	gint leave_window (GdkEventCrossing*);
	gint current_dialog_vanished (GdkEventAny*);

	void check_meta_numlock (char keycode, guint mod, string modname);
};

#endif /* __ardour_keyboard_h__ */
