/*
    Copyright (C) 20002-2004 Paul Davis 

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

/* This file contains any ARDOUR_UI methods that require knowledge of
   the editor, and exists so that no compilation dependency exists
   between the main ARDOUR_UI modules and the PublicEditor class. This
   is to cut down on the nasty compile times for both these classes.
*/

#include <pbd/pathscanner.h>

#include "ardour_ui.h"
#include "public_editor.h"
#include "audio_clock.h"
#include "editor.h"
#include "actions.h"

#include <ardour/session.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtk;
using namespace Gtkmm2ext;

int
ARDOUR_UI::create_editor ()

{
	try {
		editor = new Editor (*engine);
	}

	catch (failed_constructor& err) {
		return -1;
	}

	editor->Realized.connect (mem_fun (*this, &ARDOUR_UI::editor_realized));

	return 0;
}

void
ARDOUR_UI::install_actions ()
{
	Glib::RefPtr<ActionGroup> main_actions = ActionGroup::create (X_("Main"));
	Glib::RefPtr<Action> act;

	/* menus + submenus that need action items */

	ActionManager::register_action (main_actions, X_("Session"), _("Session"));
	ActionManager::register_action (main_actions, X_("Export"), _("Export"));
	ActionManager::register_action (main_actions, X_("Cleanup"), _("Cleanup"));
	ActionManager::register_action (main_actions, X_("Sync"), _("Sync"));
	ActionManager::register_action (main_actions, X_("Options"), _("Options"));
	ActionManager::register_action (main_actions, X_("TransportOptions"), _("Options"));

	/* the real actions */

	act = ActionManager::register_action (main_actions, X_("New"), _("New"),  bind (mem_fun(*this, &ARDOUR_UI::new_session), false, string ()));

	ActionManager::register_action (main_actions, X_("Open"), _("Open"),  mem_fun(*this, &ARDOUR_UI::open_session));
	ActionManager::register_action (main_actions, X_("Recent"), _("Recent"),  mem_fun(*this, &ARDOUR_UI::open_recent_session));
	act = ActionManager::register_action (main_actions, X_("Close"), _("Close"),  mem_fun(*this, &ARDOUR_UI::close_session));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("AddTrackBus"), _("Add Track/Bus"),  mem_fun(*this, &ARDOUR_UI::add_route));
	ActionManager::session_sensitive_actions.push_back (act);

	
	/* <CMT Additions> */

	PathScanner scanner;
	vector<string*>* results = scanner (getenv ("PATH"), "AniComp", false, false);

	if (results) {
		if (!results->empty()) {
			act = ActionManager::register_action (main_actions, X_("aniConnect"), _("Connect"),  (mem_fun (*editor, &PublicEditor::connect_to_image_compositor)));
			ActionManager::session_sensitive_actions.push_back (act);
		}
		delete results;
	}

	/* </CMT Additions> */

	act = ActionManager::register_action (main_actions, X_("Snapshot"), _("Snapshot"),  mem_fun(*this, &ARDOUR_UI::snapshot_session));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("SaveTemplate"), _("Save Template..."),  mem_fun(*this, &ARDOUR_UI::save_template));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("ExportSession"), _("Export session to audiofile..."),  mem_fun (*editor, &PublicEditor::export_session));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("ExportRange"), _("Export range to audiofile..."),  mem_fun (*editor, &PublicEditor::export_selection));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("Export"), _("Export"));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (main_actions, X_("CleanupUnused"), _("Cleanup unused sources"),  mem_fun (*(ARDOUR_UI::instance()), &ARDOUR_UI::cleanup));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (main_actions, X_("FlushWastebasket"), _("Flush wastebasket"),  mem_fun (*(ARDOUR_UI::instance()), &ARDOUR_UI::flush_trash));
	ActionManager::session_sensitive_actions.push_back (act);
	
	/* JACK actions for controlling ... JACK */

	Glib::RefPtr<ActionGroup> jack_actions = ActionGroup::create (X_("JACK"));
	ActionManager::register_action (jack_actions, X_("JACK"), _("JACK"));
	ActionManager::register_action (jack_actions, X_("Latency"), _("Latency"));
	
	act = ActionManager::register_action (jack_actions, X_("JACKReconnect"), _("Reconnect"), mem_fun (*(ARDOUR_UI::instance()), &ARDOUR_UI::reconnect_to_jack));
	ActionManager::jack_opposite_sensitive_actions.push_back (act);

	act = ActionManager::register_action (jack_actions, X_("JACKDisconnect"), _("Disconnect"), mem_fun (*(ARDOUR_UI::instance()), &ARDOUR_UI::disconnect_from_jack));
	ActionManager::jack_sensitive_actions.push_back (act);
	
	RadioAction::Group jack_latency_group;
	
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency32"), X_("32"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 32));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency64"), X_("64"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 64));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency128"), X_("128"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 128));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency256"), X_("256"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 256));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency512"), X_("512"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 512));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency1024"), X_("1024"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 1024));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency2048"), X_("2048"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 2048));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency4096"), X_("4096"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 4096));
	ActionManager::jack_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (jack_actions, jack_latency_group, X_("JACKLatency8192"), X_("8192"), bind (mem_fun(*this, &ARDOUR_UI::set_jack_buffer_size), (jack_nframes_t) 8192));
	ActionManager::jack_sensitive_actions.push_back (act);
	
	/* these actions are intended to be shared across all windows */
	
	common_actions = ActionGroup::create (X_("Common"));
	ActionManager::register_action (main_actions, X_("Windows"), _("Windows"));
	ActionManager::register_action (common_actions, X_("Start-Prefix"), _("start prefix"), mem_fun(*this, &ARDOUR_UI::start_keyboard_prefix));
	ActionManager::register_action (common_actions, X_("Quit"), _("Quit"), (mem_fun(*this, &ARDOUR_UI::finish)));

        /* windows visibility actions */

	ActionManager::register_action (common_actions, X_("GotoEditor"), _("Editor"),  mem_fun(*this, &ARDOUR_UI::goto_editor_window));
	ActionManager::register_action (common_actions, X_("GotoMixer"), _("Mixer"),  mem_fun(*this, &ARDOUR_UI::goto_mixer_window));
	ActionManager::register_toggle_action (common_actions, X_("ToggleSoundFileBrowser"), _("Sound File Browser"), mem_fun(*this, &ARDOUR_UI::toggle_sound_file_browser));
	ActionManager::register_toggle_action (common_actions, X_("ToggleOptionsEditor"), _("Options Editor"), mem_fun(*this, &ARDOUR_UI::toggle_options_window));
	act = ActionManager::register_toggle_action (common_actions, X_("ToggleInspector"), _("Track/Bus Inspector"), mem_fun(*this, &ARDOUR_UI::toggle_route_params_window));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (common_actions, X_("ToggleConnections"), _("Connections"), mem_fun(*this, &ARDOUR_UI::toggle_connection_editor));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (common_actions, X_("ToggleLocations"), _("Locations"), mem_fun(*this, &ARDOUR_UI::toggle_location_window));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (common_actions, X_("ToggleBigClock"), _("Big Clock"), mem_fun(*this, &ARDOUR_UI::toggle_big_clock_window));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_action (common_actions, X_("About"), _("About"),  mem_fun(*this, &ARDOUR_UI::show_splash));
	
	act = ActionManager::register_action (common_actions, X_("AddAudioTrack"), _("add audio track"), bind (mem_fun(*this, &ARDOUR_UI::session_add_audio_track), 1, 1));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("AddAudioBus"), _("add audio bus"), bind (mem_fun(*this, &ARDOUR_UI::session_add_audio_bus), 1, 1));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("Save"), _("Save"),  bind (mem_fun(*this, &ARDOUR_UI::save_state), string("")));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("RemoveLastCapture"), _("remove last capture"), mem_fun(*this, &ARDOUR_UI::remove_last_capture));
	ActionManager::session_sensitive_actions.push_back (act);

	Glib::RefPtr<ActionGroup> transport_actions = ActionGroup::create (X_("Transport"));

	/* do-nothing action for the "transport" menu bar item */

	ActionManager::register_action (transport_actions, X_("Transport"), _("Transport"));

	/* these two are not used by key bindings, instead use ToggleRoll for that. these two do show up in
	   menus and via button proxies.
	*/
	
	act = ActionManager::register_action (transport_actions, X_("Stop"), _("stop"), mem_fun(*this, &ARDOUR_UI::transport_stop));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("Roll"), _("roll"), mem_fun(*this, &ARDOUR_UI::transport_roll));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);

	ActionManager::register_action (transport_actions, X_("ToggleRoll"), _("start/stop"), bind (mem_fun (*editor, &PublicEditor::toggle_playback), false));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	ActionManager::register_action (transport_actions, X_("ToggleRollForgetCapture"), _("stop + forget capture"), bind (mem_fun(*editor, &PublicEditor::toggle_playback), true));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("Loop"), _("play loop range"), mem_fun(*this, &ARDOUR_UI::toggle_session_auto_loop));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("PlaySelection"), _("play selection"), mem_fun(*this, &ARDOUR_UI::transport_play_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);

	act = ActionManager::register_action (transport_actions, X_("Record"), _("enable record"), mem_fun(*this, &ARDOUR_UI::transport_record));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("Rewind"), _("rewind"), bind (mem_fun(*this, &ARDOUR_UI::transport_rewind), 0));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("RewindSlow"), _("rewind (slow)"), bind (mem_fun(*this, &ARDOUR_UI::transport_rewind), -1));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("RewindFast"), _("rewind (fast)"), bind (mem_fun(*this, &ARDOUR_UI::transport_rewind), 1));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("Forward"), _("forward"), bind (mem_fun(*this, &ARDOUR_UI::transport_forward), 0));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("ForwardSlow"), _("forward (slow)"), bind (mem_fun(*this, &ARDOUR_UI::transport_forward), -1));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("ForwardFast"), _("forward (fast)"), bind (mem_fun(*this, &ARDOUR_UI::transport_forward), 1));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("GotoStart"), _("goto start"), mem_fun(*this, &ARDOUR_UI::transport_goto_start));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_action (transport_actions, X_("GotoEnd"), _("goto end"), mem_fun(*this, &ARDOUR_UI::transport_goto_end));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);

	/* XXX the newline in the displayed names of these action is really wrong, but its because we want the button
	   that proxies for these action to be more compact. It would be nice to find a way to override the action
	   name appearance on the buttons.
	*/

	act = ActionManager::register_toggle_action (transport_actions, X_("TogglePunchIn"), _("punch\nin"), mem_fun(*this, &ARDOUR_UI::toggle_punch_in));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (transport_actions, X_("TogglePunchOut"), _("punch\nout"), mem_fun(*this, &ARDOUR_UI::toggle_punch_out));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (transport_actions, X_("ToggleClick"), _("click"), mem_fun(*this, &ARDOUR_UI::toggle_click));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (transport_actions, X_("ToggleAutoInput"), _("auto\ninput"), mem_fun(*this, &ARDOUR_UI::toggle_auto_input));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (transport_actions, X_("ToggleAutoPlay"), _("auto\nplay"), mem_fun(*this, &ARDOUR_UI::toggle_auto_play));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (transport_actions, X_("ToggleAutoReturn"), _("auto\nreturn"), mem_fun(*this, &ARDOUR_UI::toggle_auto_return));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::transport_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (transport_actions, X_("ToggleTimeMaster"), _("time\nmaster"), mem_fun(*this, &ARDOUR_UI::toggle_time_master));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (common_actions, X_("SendAllMidiFeedback"), _("send all midi feedback"), mem_fun(*this, &ARDOUR_UI::send_all_midi_feedback));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack1"), _("toggle record enable track1"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  0U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack2"), _("toggle record enable track2"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  1U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack3"), _("toggle record enable track3"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  2U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack4"), _("toggle record enable track4"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  3U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack5"), _("toggle record enable track5"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  4U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack6"), _("toggle record enable track6"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  5U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack7"), _("toggle record enable track7"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  6U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack8"), _("toggle record enable track8"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  7U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack9"), _("toggle record enable track9"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable),  8U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack10"), _("toggle record enable track10"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 9U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack11"), _("toggle record enable track11"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 10U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack12"), _("toggle record enable track12"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 11U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack13"), _("toggle record enable track13"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 12U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack14"), _("toggle record enable track14"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 13U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack15"), _("toggle record enable track15"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 14U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack16"), _("toggle record enable track16"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 15U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack17"), _("toggle record enable track17"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 16U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack18"), _("toggle record enable track18"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 17U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack19"), _("toggle record enable track19"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 18U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack20"), _("toggle record enable track20"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 19U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack21"), _("toggle record enable track21"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 20U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack22"), _("toggle record enable track22"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 21U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack23"), _("toggle record enable track23"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 22U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack24"), _("toggle record enable track24"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 23U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack25"), _("toggle record enable track25"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 24U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack26"), _("toggle record enable track26"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 25U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack27"), _("toggle record enable track27"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 26U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack28"), _("toggle record enable track28"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 27U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack29"), _("toggle record enable track29"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 28U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack30"), _("toggle record enable track30"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 29U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack31"), _("toggle record enable track31"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 30U));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (common_actions, X_("ToggleRecordEnableTrack32"), _("toggle record enable track32"), bind (mem_fun(*this, &ARDOUR_UI::toggle_record_enable), 31U));
	ActionManager::session_sensitive_actions.push_back (act);

	Glib::RefPtr<ActionGroup> shuttle_actions = ActionGroup::create ("ShuttleActions");
	
	shuttle_actions->add (Action::create (X_("SetShuttleUnitsPercentage"), _("Percentage")), bind (mem_fun(*this, &ARDOUR_UI::set_shuttle_units), Percentage));
	shuttle_actions->add (Action::create (X_("SetShuttleUnitsSemitones"), _("Semitones")), bind (mem_fun(*this, &ARDOUR_UI::set_shuttle_units), Semitones));

	Glib::RefPtr<ActionGroup> option_actions = ActionGroup::create ("options");

	act = ActionManager::register_toggle_action (option_actions, X_("SendMTC"), _("Send MTC"), mem_fun (*this, &ARDOUR_UI::toggle_send_mtc));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("SendMMC"), _("Send MMC"), mem_fun (*this, &ARDOUR_UI::toggle_send_mtc));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("UseMMC"), _("Use MMC"), mem_fun (*this, &ARDOUR_UI::toggle_use_mmc));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("SendMIDIfeedback"), _("Send MIDI feedback"), mem_fun (*this, &ARDOUR_UI::toggle_send_midi_feedback));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("UseMIDIcontrol"), _("Use MIDI control"), mem_fun (*this, &ARDOUR_UI::toggle_use_midi_control));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (option_actions, X_("AutoConnectNewTrackInputsToHardware"), _("Connect newtrack inputs to hardware"), mem_fun (*this, &ARDOUR_UI::toggle_AutoConnectNewTrackInputsToHardware));
	ActionManager::session_sensitive_actions.push_back (act);

	RadioAction::Group connect_outputs_group;

	act = ActionManager::register_radio_action (option_actions, connect_outputs_group, X_("AutoConnectNewTrackOutputsToHardware"), _("Connect new track outputs to hardware"), mem_fun (*this, &ARDOUR_UI::toggle_AutoConnectNewTrackOutputsToHardware));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (option_actions, connect_outputs_group, X_("AutoConnectNewTrackOutputsToMaster"), _("Connect new track outputs to master"), mem_fun (*this, &ARDOUR_UI::toggle_AutoConnectNewTrackOutputsToMaster));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (option_actions, connect_outputs_group, X_("ManuallyConnectNewTrackOutputs"), _("Manually connect new track outputs"), mem_fun (*this, &ARDOUR_UI::toggle_ManuallyConnectNewTrackOutputs));
	ActionManager::session_sensitive_actions.push_back (act);

	RadioAction::Group monitoring_group;

	act = ActionManager::register_radio_action (option_actions, monitoring_group, X_("UseHardwareMonitoring"), _("Hardware monitoring"), mem_fun (*this, &ARDOUR_UI::toggle_UseHardwareMonitoring));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (option_actions, monitoring_group, X_("UseSoftwareMonitoring"), _("Software monitoring"), mem_fun (*this, &ARDOUR_UI::toggle_UseSoftwareMonitoring));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (option_actions, monitoring_group, X_("UseExternalMonitoring"), _("External monitoring"), mem_fun (*this, &ARDOUR_UI::toggle_UseExternalMonitoring));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (option_actions, X_("StopPluginsWithTransport"), _("Stop plugins with transport"), mem_fun (*this, &ARDOUR_UI::toggle_StopPluginsWithTransport));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("RunPluginsWhileRecording"), _("Run plugins while recording"), mem_fun (*this, &ARDOUR_UI::toggle_RunPluginsWhileRecording));
	ActionManager::session_sensitive_actions.push_back (act);
	
	act = ActionManager::register_toggle_action (option_actions, X_("VerifyRemoveLastCapture"), _("Verify remove last capture"), mem_fun (*this, &ARDOUR_UI::toggle_VerifyRemoveLastCapture));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("StopRecordingOnXrun"), _("Stop recording on xrun"), mem_fun (*this, &ARDOUR_UI::toggle_StopRecordingOnXrun));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("StopTransportAtEndOfSession"), _("Stop transport at session end"), mem_fun (*this, &ARDOUR_UI::toggle_StopTransportAtEndOfSession));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (option_actions, X_("GainReduceFastTransport"), _("-12dB gain reduce ffwd/rewind"), mem_fun (*this, &ARDOUR_UI::toggle_GainReduceFastTransport));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (option_actions, X_("LatchedSolo"), _("Latched solo"), mem_fun (*this, &ARDOUR_UI::toggle_LatchedSolo));
	ActionManager::session_sensitive_actions.push_back (act);

	RadioAction::Group solo_group;

	act = ActionManager::register_radio_action (option_actions, solo_group, X_("SoloInPlace"), _("Solo in-place"), mem_fun (*this, &ARDOUR_UI::toggle_SoloViaBus));
	act = ActionManager::register_radio_action (option_actions, solo_group, X_("SoloViaBus"), _("Solo via bus"), mem_fun (*this, &ARDOUR_UI::toggle_SoloViaBus));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (option_actions, X_("AutomaticallyCreateCrossfades"), _("Automatically create crossfades"), mem_fun (*this, &ARDOUR_UI::toggle_AutomaticallyCreateCrossfades));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (option_actions, X_("UnmuteNewFullCrossfades"), _("Unmute new full crossfades"), mem_fun (*this, &ARDOUR_UI::toggle_UnmuteNewFullCrossfades));
	ActionManager::session_sensitive_actions.push_back (act);


#ifdef NEW_ACTIONS
	act = ActionManager::register_action (option_actions, X_("SetRegionLayerMode", _("SetRegionLayerMode"), mem_fun (*this, &ARDOUR_UI::toggle_SetRegionLayerMode)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (option_actions, X_("SetCrossfadeModel", _("SetCrossfadeModel"), mem_fun (*this, &ARDOUR_UI::toggle_SetCrossfadeModel)));
	ActionManager::session_sensitive_actions.push_back (act);
	
#endif	
	
	ActionManager::add_action_group (shuttle_actions);
	ActionManager::add_action_group (option_actions);
	ActionManager::add_action_group (jack_actions);
	ActionManager::add_action_group (transport_actions);
	ActionManager::add_action_group (main_actions);
	ActionManager::add_action_group (common_actions);
}

void
ARDOUR_UI::build_menu_bar ()
{
	menu_bar = dynamic_cast<MenuBar*> (ActionManager::get_widget (X_("/Main")));
	menu_bar->set_name ("MainMenuBar");

	wall_clock_box.add (wall_clock_label);
	wall_clock_box.set_name ("WallClock");
	wall_clock_label.set_name ("WallClock");

	disk_space_box.add (disk_space_label);
	disk_space_box.set_name ("WallClock");
	disk_space_label.set_name ("WallClock");

	cpu_load_box.add (cpu_load_label);
	cpu_load_box.set_name ("CPULoad");
	cpu_load_label.set_name ("CPULoad");

	buffer_load_box.add (buffer_load_label);
	buffer_load_box.set_name ("BufferLoad");
	buffer_load_label.set_name ("BufferLoad");

//	disk_rate_box.add (disk_rate_label);
//	disk_rate_box.set_name ("DiskRate");
//	disk_rate_label.set_name ("DiskRate");

	sample_rate_box.add (sample_rate_label);
	sample_rate_box.set_name ("SampleRate");
	sample_rate_label.set_name ("SampleRate");

	menu_hbox.pack_start (*menu_bar, true, true);
	menu_hbox.pack_end (wall_clock_box, false, false, 10);
	menu_hbox.pack_end (disk_space_box, false, false, 10);
	menu_hbox.pack_end (cpu_load_box, false, false, 10);
//	menu_hbox.pack_end (disk_rate_box, false, false, 10);
	menu_hbox.pack_end (buffer_load_box, false, false, 10);
	menu_hbox.pack_end (sample_rate_box, false, false, 10);

	menu_bar_base.set_name ("MainMenuBar");
	menu_bar_base.add (menu_hbox);
}

