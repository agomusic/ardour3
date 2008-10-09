#ifndef __gtk_ardour_option_editor_h__
#define __gtk_ardour_option_editor_h__

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

#include <gtkmm/notebook.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/table.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/comboboxtext.h>

#include <ardour/session.h>

#include "ardour_dialog.h"
#include "editing.h"
#include "audio_clock.h"

class ARDOUR_UI;
class PublicEditor;
class Mixer_UI;
class IOSelector;
class GainMeter;
class PannerUI;

class OptionEditor : public ArdourDialog
{
  public:
	OptionEditor (ARDOUR_UI&, PublicEditor&, Mixer_UI&);
	~OptionEditor ();

	void set_session (ARDOUR::Session *);
	void save ();

  private:
	ARDOUR::Session *session;
	ARDOUR_UI& ui;
	PublicEditor& editor;
	Mixer_UI& mixer;

	Gtk::Notebook notebook;

	/* Generic */

	gint wm_close (GdkEventAny *);
	bool focus_out_event_handler (GdkEventFocus*, void (OptionEditor::*pmf)());
	void parameter_changed (const char* name);

	/* paths */

	Gtk::Table	path_table;
	Gtk::Entry	session_raid_entry;

	void setup_path_options();
	void add_session_paths ();
	void remove_session_paths ();
	void raid_path_changed ();

	/* misc */

	Gtk::VBox        misc_packer;

	Gtk::Adjustment  short_xfade_adjustment;
	Gtk::HScale      short_xfade_slider;
	Gtk::Adjustment  destructo_xfade_adjustment;
	Gtk::HScale      destructo_xfade_slider;

	void setup_misc_options();

	void short_xfade_adjustment_changed ();
	void destructo_xfade_adjustment_changed ();

	Gtk::Adjustment history_depth;
	Gtk::Adjustment saved_history_depth;
	Gtk::SpinButton     history_depth_spinner;
	Gtk::SpinButton     saved_history_depth_spinner;
	Gtk::CheckButton    limit_history_button;
	Gtk::CheckButton    save_history_button;

	void history_depth_changed();
	void saved_history_depth_changed();
	void save_history_toggled ();
	void limit_history_toggled ();

	/* Sync */

	Gtk::VBox sync_packer;

	Gtk::ComboBoxText slave_type_combo;
	AudioClock smpte_offset_clock;
	Gtk::CheckButton smpte_offset_negative_button;
	Gtk::CheckButton synced_timecode_button;

	void setup_sync_options ();

	void smpte_offset_chosen ();
	void smpte_offset_negative_clicked ();
	void synced_timecode_toggled ();

	/* MIDI */

	Gtk::VBox  midi_packer;

	Gtk::RadioButton::Group mtc_button_group;
	Gtk::RadioButton::Group mmc_button_group;
	Gtk::RadioButton::Group midi_button_group;
	Gtk::RadioButton::Group midi_clock_button_group;

	Gtk::Table      midi_port_table;
	std::vector<Gtk::Widget*> midi_port_table_widgets;
	Gtk::Adjustment mmc_receive_device_id_adjustment;
	Gtk::SpinButton mmc_receive_device_id_spinner;
	Gtk::Adjustment mmc_send_device_id_adjustment;
	Gtk::SpinButton mmc_send_device_id_spinner;
	Gtk::Button     add_midi_port_button;
	Gtk::Adjustment initial_program_change_adjustment;
	Gtk::SpinButton initial_program_change_spinner;

	void add_midi_port ();
	void remove_midi_port (MIDI::Port*);
	void redisplay_midi_ports ();

	void port_online_toggled (MIDI::Port*,Gtk::ToggleButton*);
	void port_trace_in_toggled (MIDI::Port*,Gtk::ToggleButton*);
	void port_trace_out_toggled (MIDI::Port*,Gtk::ToggleButton*);

	void mmc_port_chosen (MIDI::Port*,Gtk::RadioButton*, Gtk::Button*);
	void mtc_port_chosen (MIDI::Port*,Gtk::RadioButton*, Gtk::Button*);
	void midi_port_chosen (MIDI::Port*,Gtk::RadioButton*, Gtk::Button*);
	void midi_clock_port_chosen (MIDI::Port*,Gtk::RadioButton*, Gtk::Button*);
	bool port_removable (MIDI::Port*);

	void mmc_receive_device_id_adjusted ();
	void mmc_send_device_id_adjusted ();

	void initial_program_change_adjusted ();

	void map_port_online (MIDI::Port*, Gtk::ToggleButton*);

	void setup_midi_options();

	enum PortIndex {
		MtcIndex = 0,
		MmcIndex = 1,
		MidiIndex = 2,
		MidiClockIndex = 3
	};

	std::map<MIDI::Port*,std::vector<Gtk::RadioButton*> > port_toggle_buttons;

	/* Click */

	IOSelector*   click_io_selector;
	GainMeter* click_gpm;
	PannerUI*     click_panner;
	bool          first_click_setup;
	Gtk::HBox     click_hpacker;
	Gtk::VBox     click_packer;
	Gtk::Table    click_table;
	Gtk::Entry    click_path_entry;
	Gtk::Entry    click_emphasis_path_entry;
	Gtk::Button   click_browse_button;
	Gtk::Button   click_emphasis_browse_button;

	void setup_click_editor ();
	void clear_click_editor ();

	void click_chosen (const string & paths);
	void click_emphasis_chosen (const string & paths);

	void click_browse_clicked ();
	void click_emphasis_browse_clicked ();

	void click_sound_changed ();
	void click_emphasis_sound_changed ();

	/* Auditioner */

	Gtk::VBox     audition_packer;
	Gtk::HBox     audition_hpacker;
	Gtk::Label    audition_label;
	IOSelector*   auditioner_io_selector;
	GainMeter* auditioner_gpm;
	PannerUI* auditioner_panner;

	void setup_auditioner_editor ();
	void clear_auditioner_editor ();
	void connect_audition_editor ();

	/* keyboard/mouse */

	Gtk::Table keyboard_mouse_table;
	Gtk::ComboBoxText keyboard_layout_selector;
	Gtk::ComboBoxText edit_modifier_combo;
	Gtk::ComboBoxText delete_modifier_combo;
	Gtk::ComboBoxText snap_modifier_combo;
	Gtk::Adjustment delete_button_adjustment;
	Gtk::SpinButton delete_button_spin;
	Gtk::Adjustment edit_button_adjustment;
	Gtk::SpinButton edit_button_spin;

	std::map<std::string,std::string> bindings_files;

	void setup_keyboard_options ();
	void delete_modifier_chosen ();
	void edit_modifier_chosen ();
	void snap_modifier_chosen ();
	void edit_button_changed ();
	void delete_button_changed ();
	void bindings_changed ();

	void fixup_combo_size (Gtk::ComboBoxText&, std::vector<std::string>& strings);
};

#endif /* __gtk_ardour_option_editor_h__ */


