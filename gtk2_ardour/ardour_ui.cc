/*
    Copyright (C) 1999-2002 Paul Davis 

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

#include <algorithm>
#include <cmath>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>
#include <fstream>

#include <iostream>

#include <gtkmm/messagedialog.h>
#include <gtkmm/accelmap.h>

#include <pbd/error.h>
#include <pbd/compose.h>
#include <pbd/basename.h>
#include <pbd/pathscanner.h>
#include <pbd/failed_constructor.h>
#include <gtkmm2ext/gtk_ui.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/click_box.h>
#include <gtkmm2ext/fastmeter.h>
#include <gtkmm2ext/stop_signal.h>
#include <gtkmm2ext/popup.h>

#include <midi++/port.h>
#include <midi++/mmc.h>

#include <ardour/ardour.h>
#include <ardour/port.h>
#include <ardour/audioengine.h>
#include <ardour/playlist.h>
#include <ardour/utils.h>
#include <ardour/diskstream.h>
#include <ardour/filesource.h>
#include <ardour/recent_sessions.h>
#include <ardour/session_diskstream.h>
#include <ardour/port.h>
#include <ardour/audio_track.h>

#include "actions.h"
#include "ardour_ui.h"
#include "public_editor.h"
#include "audio_clock.h"
#include "keyboard.h"
#include "mixer_ui.h"
#include "prompter.h"
#include "opts.h"
#include "keyboard_target.h"
#include "add_route_dialog.h"
#include "new_session_dialog.h"
#include "about.h"
#include "utils.h"
#include "gui_thread.h"
#include "color_manager.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtkmm2ext;
using namespace Gtk;
using namespace sigc;

ARDOUR_UI *ARDOUR_UI::theArdourUI = 0;

sigc::signal<void,bool> ARDOUR_UI::Blink;
sigc::signal<void>      ARDOUR_UI::RapidScreenUpdate;
sigc::signal<void>      ARDOUR_UI::SuperRapidScreenUpdate;
sigc::signal<void,jack_nframes_t> ARDOUR_UI::Clock;

ARDOUR_UI::ARDOUR_UI (int *argcp, char **argvp[], string rcfile)

	: Gtkmm2ext::UI ("ardour", argcp, argvp, rcfile),
	  
	  primary_clock (X_("TransportClockDisplay"), true, false, true),
	  secondary_clock (X_("SecondaryClockDisplay"), true, false, true),
	  preroll_clock (X_("PreRollClock"), true, true),
	  postroll_clock (X_("PostRollClock"), true, true),

	  /* adjuster table */

	  adjuster_table (3, 3),

	  /* preroll stuff */

	  preroll_button (_("pre\nroll")),
	  postroll_button (_("post\nroll")),

	  /* big clock */

	  big_clock ("BigClockDisplay", true),

	  /* transport */

	  time_master_button (_("time\nmaster")),

	  shuttle_units_button (_("% ")),

	  punch_in_button (_("punch\nin")),
	  punch_out_button (_("punch\nout")),
	  auto_return_button (_("auto\nreturn")),
	  auto_play_button (_("auto\nplay")),
	  auto_input_button (_("auto\ninput")),
	  click_button (_("click")),
	  auditioning_alert_button (_("AUDITIONING")),
	  solo_alert_button (_("SOLO")),
	  shown_flag (false)

{
	using namespace Gtk::Menu_Helpers;

	Gtkmm2ext::init();
	
	about = 0;

	if (theArdourUI == 0) {
		theArdourUI = this;
	}

	ActionManager::init ();

	/* load colors */

	color_manager = new ColorManager();

	std::string color_file = ARDOUR::find_config_file("ardour.colors");
	
	color_manager->load (color_file);

	m_new_session_dialog = new NewSessionDialog();
	editor = 0;
	mixer = 0;
	session = 0;
	_session_is_new = false;
	big_clock_window = 0;
	session_selector_window = 0;
	last_key_press_time = 0;
	connection_editor = 0;
	add_route_dialog = 0;
	route_params = 0;
	option_editor = 0;
	location_ui = 0;
	sfdb = 0;
	open_session_selector = 0;
	have_configure_timeout = false;
	have_disk_overrun_displayed = false;
	have_disk_underrun_displayed = false;
	_will_create_new_session_automatically = false;
	session_loaded = false;
	last_speed_displayed = -1.0f;

	last_configure_time.tv_sec = 0;
	last_configure_time.tv_usec = 0;

	shuttle_grabbed = false;
	shuttle_fract = 0.0;
	shuttle_max_speed = 8.0f;

	set_shuttle_units (Percentage);
	set_shuttle_behaviour (Sprung);

	shuttle_style_menu = 0;
	shuttle_unit_menu = 0;

	gettimeofday (&last_peak_grab, 0);
	gettimeofday (&last_shuttle_request, 0);

	ARDOUR::DiskStream::CannotRecordNoInput.connect (mem_fun(*this, &ARDOUR_UI::cannot_record_no_input));
	ARDOUR::DiskStream::DeleteSources.connect (mem_fun(*this, &ARDOUR_UI::delete_sources_in_the_right_thread));
	ARDOUR::DiskStream::DiskOverrun.connect (mem_fun(*this, &ARDOUR_UI::disk_overrun_handler));
	ARDOUR::DiskStream::DiskUnderrun.connect (mem_fun(*this, &ARDOUR_UI::disk_underrun_handler));

	/* handle pending state with a dialog */

	ARDOUR::Session::AskAboutPendingState.connect (mem_fun(*this, &ARDOUR_UI::pending_state_dialog));

	/* have to wait for AudioEngine and Configuration before proceeding */
}

void
ARDOUR_UI::cannot_record_no_input (DiskStream* ds)
{
	ENSURE_GUI_THREAD (bind (mem_fun(*this, &ARDOUR_UI::cannot_record_no_input), ds));
	
	string msg = string_compose (_("\
You cannot record-enable\n\
track %1\n\
because it has no input connections.\n\
You would be wasting space recording silence."),
			      ds->name());

	MessageDialog message (*editor, msg);
	message.run ();
}

void
ARDOUR_UI::set_engine (AudioEngine& e)
{
	engine = &e;

	engine->Stopped.connect (mem_fun(*this, &ARDOUR_UI::engine_stopped));
	engine->Running.connect (mem_fun(*this, &ARDOUR_UI::engine_running));
	engine->Halted.connect (mem_fun(*this, &ARDOUR_UI::engine_halted));
	engine->SampleRateChanged.connect (mem_fun(*this, &ARDOUR_UI::update_sample_rate));

	_tooltips.enable();

	keyboard = new Keyboard;

	string meter_path;

	meter_path = ARDOUR::find_data_file("v_meter_strip.xpm", "pixmaps");
	if (meter_path.empty()) {
		error << _("no vertical meter strip image found") << endmsg;
		exit (1);
	}
 	FastMeter::set_vertical_xpm (meter_path);

	meter_path = ARDOUR::find_data_file("h_meter_strip.xpm", "pixmaps");
	if (meter_path.empty()) {
		error << _("no horizontal meter strip image found") << endmsg;
		exit (1);
	}
 	FastMeter::set_horizontal_xpm (meter_path);

	if (setup_windows ()) {
		throw failed_constructor ();
	}

	if (GTK_ARDOUR::show_key_actions) {
		vector<string> names;
		vector<string> paths;
		vector<string> keys;
		vector<AccelKey> bindings;

		ActionManager::get_all_actions (names, paths, keys, bindings);

		vector<string>::iterator n;
		vector<string>::iterator k;
		for (n = names.begin(), k = keys.begin(); n != names.end(); ++n, ++k) {
			cerr << "Action: " << (*n) << " bound to " << (*k) << endl;
		}

		exit (0);
	}

	/* start with timecode, metering enabled
	*/
	
	blink_timeout_tag = -1;

	/* the global configuration object is now valid */

	use_config ();

	/* this being a GUI and all, we want peakfiles */

	FileSource::set_build_peakfiles (true);
	FileSource::set_build_missing_peakfiles (true);

	if (Source::start_peak_thread ()) {
		throw failed_constructor();
	}

	/* start the time-of-day-clock */
	
	update_wall_clock ();
	Glib::signal_timeout().connect (mem_fun(*this, &ARDOUR_UI::update_wall_clock), 60000);

	update_disk_space ();
	update_cpu_load ();
	update_sample_rate (engine->frame_rate());

	starting.connect (mem_fun(*this, &ARDOUR_UI::startup));
	stopping.connect (mem_fun(*this, &ARDOUR_UI::shutdown));
}

ARDOUR_UI::~ARDOUR_UI ()
{
	save_ardour_state ();

	if (keyboard) {
		delete keyboard;
	}

	if (editor) {
		delete editor;
	}

	if (mixer) {
		delete mixer;
	}

	if (add_route_dialog) {
		delete add_route_dialog;
	}

	Source::stop_peak_thread ();
}

gint
ARDOUR_UI::configure_timeout ()
{
	struct timeval now;
	struct timeval diff;

	if (last_configure_time.tv_sec == 0 && last_configure_time.tv_usec == 0) {
		/* no configure events yet */
		return TRUE;
	}

	gettimeofday (&now, 0);
	timersub (&now, &last_configure_time, &diff);

	/* force a gap of 0.5 seconds since the last configure event
	 */

	if (diff.tv_sec == 0 && diff.tv_usec < 500000) {
		return TRUE;
	} else {
		have_configure_timeout = false;
		save_ardour_state ();
		return FALSE;
	}
}

gboolean
ARDOUR_UI::configure_handler (GdkEventConfigure* conf)
{
	if (have_configure_timeout) {
		gettimeofday (&last_configure_time, 0);
	} else {
		Glib::signal_timeout().connect (mem_fun(*this, &ARDOUR_UI::configure_timeout), 100);
		have_configure_timeout = true;
	}
		
	return FALSE;
}

void
ARDOUR_UI::save_ardour_state ()
{
	if (!keyboard || !mixer || !editor) {
		return;
	}
	
	/* XXX this is all a bit dubious. add_extra_xml() uses
	   a different lifetime model from add_instant_xml().
	*/

	XMLNode* node = new XMLNode (keyboard->get_state());
	Config->add_extra_xml (*node);
	Config->save_state();

	XMLNode& enode (static_cast<Stateful*>(editor)->get_state());
	XMLNode& mnode (mixer->get_state());

	if (session) {
		session->add_instant_xml(enode, session->path());
		session->add_instant_xml(mnode, session->path());
	} else {
		Config->add_instant_xml(enode, get_user_ardour_path());
		Config->add_instant_xml(mnode, get_user_ardour_path());
	}

	/* keybindings */

	AccelMap::save ("ardour.saved_bindings");
}

void
ARDOUR_UI::startup ()
{
	/* Once the UI is up and running, start the audio engine. Doing
	   this before the UI is up and running can cause problems
	   when not running with SCHED_FIFO, because the amount of
	   CPU and disk work needed to get the UI started can interfere
	   with the scheduling of the audio thread.
	*/

	Glib::signal_idle().connect (mem_fun(*this, &ARDOUR_UI::start_engine));
}

void
ARDOUR_UI::finish()
{
	if (session && session->dirty()) {
		switch (ask_about_saving_session(_("quit"))) {
		case -1:
			return;
			break;
		case 1:
			/* use the default name */
			if (save_state_canfail ("")) {
				/* failed - don't quit */
				MessageDialog msg (*editor, 
					       _("\
Ardour was unable to save your session.\n\n\
If you still wish to quit, please use the\n\n\
\"Just quit\" option."));
				msg.run ();
				return;
			}
			break;
		case 0:
			break;
		}
	}

	quit ();
}

int
ARDOUR_UI::ask_about_saving_session (const string & what)
{
	ArdourDialog window (_("ardour: save session?"));
	Gtk::HBox dhbox;  // the hbox for the image and text
	Gtk::Label  prompt_label;
	Gtk::Image* dimage = manage (new Gtk::Image(Stock::DIALOG_WARNING,  Gtk::ICON_SIZE_DIALOG));

	string msg;

	msg = string_compose(_("Don't %1"), what);
	window.add_button (msg, RESPONSE_REJECT);
	msg = string_compose(_("Just %1"), what);
	window.add_button (msg, RESPONSE_APPLY);
	msg = string_compose(_("Save and %1"), what);
	window.add_button (msg, RESPONSE_ACCEPT);

	window.set_default_response (RESPONSE_ACCEPT);

	Gtk::Button noquit_button (msg);
	noquit_button.set_name ("EditorGTKButton");

	string prompt;
	string type;

	if (session->snap_name() == session->name()) {
		type = _("session");
	} else {
		type = _("snapshot");
	}
	prompt = string_compose(_("The %1\"%2\"\nhas not been saved.\n\nAny changes made this time\nwill be lost unless you save it.\n\nWhat do you want to do?"), 
			 type, session->snap_name());
	
	prompt_label.set_text (prompt);
	prompt_label.set_name (X_("PrompterLabel"));
	prompt_label.set_alignment(ALIGN_LEFT, ALIGN_TOP);

	dimage->set_alignment(ALIGN_CENTER, ALIGN_TOP)
;
	dhbox.set_homogeneous (false);
	dhbox.pack_start (*dimage, false, false, 5);
	dhbox.pack_start (prompt_label, true, false, 5);
	window.get_vbox()->pack_start (dhbox);

	window.set_name (_("Prompter"));
	window.set_position (Gtk::WIN_POS_MOUSE);
	window.set_modal (true);
	window.set_resizable (false);
	window.show_all ();

	save_the_session = 0;

	editor->ensure_float (window);

	ResponseType r = (ResponseType) window.run();

	window.hide ();

	switch (r) {
	case RESPONSE_ACCEPT: // save and get out of here
		return 1;
	case RESPONSE_APPLY:  // get out of here
		return 0;
	default:
		break;
	}

	return -1;
}
	
gint
ARDOUR_UI::every_second ()
{
	update_cpu_load ();
	update_buffer_load ();
	update_disk_space ();
	return TRUE;
}

gint
ARDOUR_UI::every_point_one_seconds ()
{
	update_speed_display ();
	RapidScreenUpdate(); /* EMIT_SIGNAL */
	return TRUE;
}

gint
ARDOUR_UI::every_point_zero_one_seconds ()
{
	SuperRapidScreenUpdate(); /* EMIT_SIGNAL */
	return TRUE;
}

void
ARDOUR_UI::update_sample_rate (jack_nframes_t ignored)
{
	char buf[32];

	ENSURE_GUI_THREAD (bind (mem_fun(*this, &ARDOUR_UI::update_sample_rate), ignored));

	if (!engine->connected()) {

		snprintf (buf, sizeof (buf), _("disconnected"));

	} else {

		jack_nframes_t rate = engine->frame_rate();
		
		if (fmod (rate, 1000.0) != 0.0) {
			snprintf (buf, sizeof (buf), _("SR: %.1f kHz / %4.1f msecs"), 
				  (float) rate/1000.0f,
				  (engine->frames_per_cycle() / (float) rate) * 1000.0f);
		} else {
			snprintf (buf, sizeof (buf), _("SR: %u kHz / %4.1f msecs"), 
				  rate/1000,
				  (engine->frames_per_cycle() / (float) rate) * 1000.0f);
		}
	}

	sample_rate_label.set_text (buf);
}

void
ARDOUR_UI::update_cpu_load ()
{
	char buf[32];
	snprintf (buf, sizeof (buf), _("DSP Load: %.1f%%"), engine->get_cpu_load());
	cpu_load_label.set_text (buf);
}

void
ARDOUR_UI::update_buffer_load ()
{
	char buf[64];

	if (session) {
		snprintf (buf, sizeof (buf), _("Buffers p:%" PRIu32 "%% c:%" PRIu32 "%%"), 
			  session->playback_load(), session->capture_load());
		buffer_load_label.set_text (buf);
	} else {
		buffer_load_label.set_text ("");
	}
}

void
ARDOUR_UI::count_recenabled_diskstreams (DiskStream& ds)
{
	if (ds.record_enabled()) {
		rec_enabled_diskstreams++;
	}
}

void
ARDOUR_UI::update_disk_space()
{
	if (session == 0) {
		return;
	}

	jack_nframes_t frames = session->available_capture_duration();
	char buf[64];

	if (frames == max_frames) {
		strcpy (buf, _("space: 24hrs+"));
	} else {
		int hrs;
		int mins;
		int secs;
		jack_nframes_t fr = session->frame_rate();
		
		if (session->actively_recording()){
			
			rec_enabled_diskstreams = 0;
			session->foreach_diskstream (this, &ARDOUR_UI::count_recenabled_diskstreams);
			
			if (rec_enabled_diskstreams) {
				frames /= rec_enabled_diskstreams;
			}
			
		} else {
			
			/* hmmm. shall we divide by the route count? or the diskstream count?
			   or what? for now, do nothing ...
			*/
			
		}
		
		hrs  = frames / (fr * 3600);
		frames -= hrs * fr * 3600;
		mins = frames / (fr * 60);
		frames -= mins * fr * 60;
		secs = frames / fr;
		
		snprintf (buf, sizeof(buf), _("space: %02dh:%02dm:%02ds"), hrs, mins, secs);
	}

	disk_space_label.set_text (buf);
}		  

gint
ARDOUR_UI::update_wall_clock ()
{
	time_t now;
	struct tm *tm_now;
	char buf[16];

	time (&now);
	tm_now = localtime (&now);

	sprintf (buf, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	wall_clock_label.set_text (buf);

	return TRUE;
}
void
ARDOUR_UI::control_methods_adjusted ()

{
	int which_method;

	which_method = (int) online_control_button->adjustment.get_value();
	switch (which_method) {
	case 0:
		allow_mmc_and_local ();
		break;
	case 1:
		allow_mmc_only ();
		break;
	case 2:
		allow_local_only ();
		break;
	default:
		fatal << _("programming error: impossible control method") << endmsg;
	}
}
	

void
ARDOUR_UI::mmc_device_id_adjusted ()

{
#if 0
	if (mmc) {
		int dev_id = (int) mmc_id_button->adjustment.get_value();
		mmc->set_device_id (dev_id);
	}
#endif
}

gint
ARDOUR_UI::session_menu (GdkEventButton *ev)
{
	session_popup_menu->popup (0, 0);
	return TRUE;
}

void
ARDOUR_UI::redisplay_recent_sessions ()
{
	vector<string *> *sessions;
	vector<string *>::iterator i;
	RecentSessionsSorter cmp;
	
	recent_session_display.set_model (Glib::RefPtr<TreeModel>(0));
	recent_session_model->clear ();

	RecentSessions rs;
	ARDOUR::read_recent_sessions (rs);

	if (rs.empty()) {
		recent_session_display.set_model (recent_session_model);
		return;
	}

	/* sort them alphabetically */
	sort (rs.begin(), rs.end(), cmp);
	sessions = new vector<string*>;

	for (RecentSessions::iterator i = rs.begin(); i != rs.end(); ++i) {
		sessions->push_back (new string ((*i).second));
	}

	for (i = sessions->begin(); i != sessions->end(); ++i) {

		vector<string*>* states;
		vector<const gchar*> item;
		string fullpath = *(*i);
		
		/* remove any trailing / */

		if (fullpath[fullpath.length()-1] == '/') {
			fullpath = fullpath.substr (0, fullpath.length()-1);
		}

		/* now get available states for this session */

		if ((states = Session::possible_states (fullpath)) == 0) {
			/* no state file? */
			continue;
		}

		TreeModel::Row row = *(recent_session_model->append());

		row[recent_session_columns.visible_name] = PBD::basename (fullpath);
		row[recent_session_columns.fullpath] = fullpath;

		if (states->size() > 1) {

			/* add the children */
			
			for (vector<string*>::iterator i2 = states->begin(); i2 != states->end(); ++i2) {
				
				TreeModel::Row child_row = *(recent_session_model->append (row.children()));

				child_row[recent_session_columns.visible_name] = **i2;
				child_row[recent_session_columns.fullpath] = fullpath;

				delete *i2;
			}
		}

		delete states;
	}

	recent_session_display.set_model (recent_session_model);
	delete sessions;
}

void
ARDOUR_UI::build_session_selector ()
{
	session_selector_window = new ArdourDialog ("session selector");
	
	Gtk::ScrolledWindow *scroller = manage (new Gtk::ScrolledWindow);
	
	session_selector_window->add_button (Stock::CANCEL, RESPONSE_CANCEL);
	session_selector_window->add_button (Stock::OPEN, RESPONSE_ACCEPT);
	session_selector_window->set_default_response (RESPONSE_ACCEPT);
	recent_session_model = TreeStore::create (recent_session_columns);
	recent_session_display.set_model (recent_session_model);
	recent_session_display.append_column (_("Recent Sessions"), recent_session_columns.visible_name);
	recent_session_display.set_headers_visible (false);
	recent_session_display.get_selection()->set_mode (SELECTION_SINGLE);

	recent_session_display.signal_row_activated().connect (mem_fun (*this, &ARDOUR_UI::recent_session_row_activated));

	scroller->add (recent_session_display);
	scroller->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	session_selector_window->set_name ("SessionSelectorWindow");
	session_selector_window->set_size_request (200, 400);
	session_selector_window->get_vbox()->pack_start (*scroller);
	session_selector_window->show_all_children();
}

void
ARDOUR_UI::recent_session_row_activated (const TreePath& path, TreeViewColumn* col)
{
	session_selector_window->response (RESPONSE_ACCEPT);
}

void
ARDOUR_UI::open_recent_session ()
{
	/* popup selector window */

	if (session_selector_window == 0) {
		build_session_selector ();
	}

	redisplay_recent_sessions ();

	ResponseType r = (ResponseType) session_selector_window->run ();

	session_selector_window->hide();

	switch (r) {
	case RESPONSE_ACCEPT:
		break;
	default:
		return;
	}

	Gtk::TreeModel::iterator i = recent_session_display.get_selection()->get_selected();

	if (i == recent_session_model->children().end()) {
		return;
	}
	
	Glib::ustring path = (*i)[recent_session_columns.fullpath];
	Glib::ustring state = (*i)[recent_session_columns.visible_name];

	_session_is_new = false;

	load_session (path, state);
}

bool
ARDOUR_UI::filter_ardour_session_dirs (const FileFilter::Info& info) 
{
	struct stat statbuf;

	if (stat (info.filename.c_str(), &statbuf) != 0) {
		return false;
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		return false;
	}

	string session_file = info.filename;
	session_file += '/';
	session_file += PBD::basename (info.filename);
	session_file += ".ardour";
	
	if (stat (session_file.c_str(), &statbuf) != 0) {
		return false;
	}

	return S_ISREG (statbuf.st_mode);
}

void
ARDOUR_UI::open_session ()
{
	/* popup selector window */

	if (open_session_selector == 0) {

		/* ardour sessions are folders */

		open_session_selector = new Gtk::FileChooserDialog (_("open session"), FILE_CHOOSER_ACTION_OPEN);
		open_session_selector->add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		open_session_selector->add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

		FileFilter session_filter;
		session_filter.add_pattern ("*.ardour");
		session_filter.set_name (_("Ardour sessions"));
		open_session_selector->add_filter (session_filter);
		open_session_selector->set_filter (session_filter);
  	}

	int response = open_session_selector->run();
	open_session_selector->hide ();

	switch (response) {
	case RESPONSE_ACCEPT:
		break;
	default:
		open_session_selector->hide();
		return;
	}

	open_session_selector->hide();
	string session_path = open_session_selector->get_filename();
	string path, name;
	bool isnew;

	if (session_path.length() > 0) {
		if (Session::find_session (session_path, path, name, isnew) == 0) {
			_session_is_new = isnew;
			load_session (path, name);
		}
	}
}


void
ARDOUR_UI::session_add_midi_track ()
{
	cerr << _("Patience is a virtue.\n");
}

void
ARDOUR_UI::session_add_audio_route (bool disk, int32_t input_channels, int32_t output_channels, ARDOUR::TrackMode mode)
{
	Route* route;

	if (session == 0) {
		warning << _("You cannot add a track without a session already loaded.") << endmsg;
		return;
	}

	try { 
		if (disk) {
			if ((route = session->new_audio_track (input_channels, output_channels, mode)) == 0) {
				error << _("could not create new audio track") << endmsg;
			}
		} else {
			if ((route = session->new_audio_route (input_channels, output_channels)) == 0) {
				error << _("could not create new audio bus") << endmsg;
			}
		}
		
#if CONTROLOUTS
		if (need_control_room_outs) {
			pan_t pans[2];
			
			pans[0] = 0.5;
			pans[1] = 0.5;
			
			route->set_stereo_control_outs (control_lr_channels);
			route->control_outs()->set_stereo_pan (pans, this);
		}
#endif /* CONTROLOUTS */
	}

	catch (...) {
		MessageDialog msg (*editor, 
				   _("There are insufficient JACK ports available\n\
to create a new track or bus.\n\
You should save Ardour, exit and\n\
restart JACK with more ports."));
		msg.run ();
	}
}

void
ARDOUR_UI::diskstream_added (DiskStream* ds)
{
}

void
ARDOUR_UI::do_transport_locate (jack_nframes_t new_position)
{
	jack_nframes_t _preroll;

	if (session) {
		_preroll = session->convert_to_frames_at (new_position, session->preroll);

		if (new_position > _preroll) {
			new_position -= _preroll;
		} else {
			new_position = 0;
		}

		session->request_locate (new_position);
	}
}

void
ARDOUR_UI::transport_goto_start ()
{
	if (session) {
		session->goto_start();

		
		/* force displayed area in editor to start no matter
		   what "follow playhead" setting is.
		*/
		
		if (editor) {
			editor->reposition_x_origin (session->current_start_frame());
		}
	}
}

void
ARDOUR_UI::transport_goto_zero ()
{
	if (session) {
		session->request_locate (0);

		
		/* force displayed area in editor to start no matter
		   what "follow playhead" setting is.
		*/
		
		if (editor) {
			editor->reposition_x_origin (0);
		}
	}
}

void
ARDOUR_UI::transport_goto_end ()
{
	if (session) {
		jack_nframes_t frame = session->current_end_frame();
		session->request_locate (frame);

		/* force displayed area in editor to start no matter
		   what "follow playhead" setting is.
		*/
		
		if (editor) {
			editor->reposition_x_origin (frame);
		}
	}
}

void
ARDOUR_UI::transport_stop ()
{
	if (!session) {
		return;
	}

	if (session->is_auditioning()) {
		session->cancel_audition ();
		return;
	}
	
	if (session->get_auto_loop()) {
		session->request_auto_loop (false);
	}
	
	session->request_stop ();
}

void
ARDOUR_UI::transport_stop_and_forget_capture ()
{
	if (session) {
		session->request_stop (true);
	}
}

void
ARDOUR_UI::remove_last_capture()
{
	if (editor) {
		editor->remove_last_capture();
	}
}

void
ARDOUR_UI::transport_record ()
{
	if (session) {
		switch (session->record_status()) {
		case Session::Disabled:
			if (session->ntracks() == 0) {
				string txt = _("Please create 1 or more track\nbefore trying to record.\nCheck the Session menu.");
				MessageDialog msg (*editor, txt);
				msg.run ();
				return;
			}
			session->maybe_enable_record ();
			break;
		case Session::Recording:
		case Session::Enabled:
			session->disable_record (true);
		}
	}
}

void
ARDOUR_UI::transport_roll ()
{
	bool rolling;

	if (!session) {
		return;
	}

	rolling = session->transport_rolling ();

	if (session->get_auto_loop()) {
		session->request_auto_loop (false);
		auto_loop_button.set_active (false);
		roll_button.set_active (true);
	} else if (session->get_play_range ()) {
		session->request_play_range (false);
		play_selection_button.set_active (false);
	} else if (rolling) {
		session->request_locate (session->last_transport_start(), true);
	}

	session->request_transport_speed (1.0f);
}

void
ARDOUR_UI::transport_loop()
{
	if (session) {
		if (session->get_auto_loop()) {
			if (session->transport_rolling()) {
				Location * looploc = session->locations()->auto_loop_location();
				if (looploc) {
					session->request_locate (looploc->start(), true);
				}
			}
		}
		else {
			session->request_auto_loop (true);
		}
	}
}

void
ARDOUR_UI::transport_play_selection ()
{
	if (!session) {
		return;
	}

	if (!session->get_play_range()) {
		session->request_stop ();
	}

	editor->play_selection ();
}

void
ARDOUR_UI::transport_rewind (int option)
{
	float current_transport_speed;
 
       	if (session) {
		current_transport_speed = session->transport_speed();
		
		if (current_transport_speed >= 0.0f) {
			switch (option) {
			case 0:
				session->request_transport_speed (-1.0f);
				break;
			case 1:
				session->request_transport_speed (-4.0f);
				break;
			case -1:
				session->request_transport_speed (-0.5f);
				break;
			}
		} else {
			/* speed up */
			session->request_transport_speed (current_transport_speed * 1.5f);
		}
	}
}

void
ARDOUR_UI::transport_forward (int option)
{
	float current_transport_speed;
	
	if (session) {
		current_transport_speed = session->transport_speed();
		
		if (current_transport_speed <= 0.0f) {
			switch (option) {
			case 0:
				session->request_transport_speed (1.0f);
				break;
			case 1:
				session->request_transport_speed (4.0f);
				break;
			case -1:
				session->request_transport_speed (0.5f);
				break;
			}
		} else {
			/* speed up */
			session->request_transport_speed (current_transport_speed * 1.5f);
		}
	}
}

void
ARDOUR_UI::toggle_monitor_enable (guint32 dstream)
{
	if (session == 0) {
		return;
	}

	DiskStream *ds;

	if ((ds = session->diskstream_by_id (dstream)) != 0) {
		Port *port = ds->io()->input (0);
		port->request_monitor_input (!port->monitoring_input());
	}
}

void
ARDOUR_UI::toggle_record_enable (guint32 dstream)
{
	if (session == 0) {
		return;
	}

	DiskStream *ds;

	if ((ds = session->diskstream_by_id (dstream)) != 0) {
		ds->set_record_enabled (!ds->record_enabled(), this);
	}
}

void
ARDOUR_UI::queue_transport_change ()
{
	Gtkmm2ext::UI::instance()->call_slot (mem_fun(*this, &ARDOUR_UI::map_transport_state));
}

void
ARDOUR_UI::map_transport_state ()
{
	float sp = session->transport_speed();

	if (sp == 1.0f) {
		transport_rolling ();
	} else if (sp < 0.0f) {
		transport_rewinding ();
	} else if (sp > 0.0f) {
		transport_forwarding ();
	} else {
		transport_stopped ();
	}
}

void
ARDOUR_UI::allow_local_only ()
{

}

void
ARDOUR_UI::allow_mmc_only ()
{

}

void
ARDOUR_UI::allow_mmc_and_local ()
{

}

void
ARDOUR_UI::GlobalClickBox::printer (char buf[32], Adjustment &adj, void *arg)
{
	snprintf (buf, sizeof(buf), "%s", ((GlobalClickBox *) arg)->strings[
		(int) adj.get_value()].c_str());
}

void
ARDOUR_UI::engine_stopped ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::engine_stopped));
	ActionManager::set_sensitive (ActionManager::jack_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::jack_opposite_sensitive_actions, true);
}


void
ARDOUR_UI::engine_running ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::engine_running));
	ActionManager::set_sensitive (ActionManager::jack_sensitive_actions, true);
	ActionManager::set_sensitive (ActionManager::jack_opposite_sensitive_actions, false);
}

void
ARDOUR_UI::engine_halted ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::engine_halted));

	ActionManager::set_sensitive (ActionManager::jack_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::jack_opposite_sensitive_actions, true);

	update_sample_rate (0);

	MessageDialog msg (*editor, 
			   _("\
JACK has either been shutdown or it\n\
disconnected Ardour because Ardour\n\
was not fast enough. You can save the\n\
session and/or try to reconnect to JACK ."));
	msg.run ();
}

int32_t
ARDOUR_UI::do_engine_start ()
{
	try { 
		engine->start();
	}

	catch (AudioEngine::PortRegistrationFailure& err) {
		engine->stop ();
		error << _("Unable to create all required ports")
		      << endmsg;
		unload_session ();
		return -1;
	}

	catch (...) {
		engine->stop ();
		error << _("Unable to start the session running")
		      << endmsg;
		unload_session ();
		return -2;
	}
	
	return 0;
}

gint
ARDOUR_UI::start_engine ()
{
	if (do_engine_start () == 0) {
		if (session && _session_is_new) {
			/* we need to retain initial visual 
			   settings for a new session 
			*/
			session->save_state ("");
		}

		/* there is too much going on, in too many threads, for us to 
		   end up with a clean session. So wait 1 second after loading,
		   and fix it up. its ugly, but until i come across a better
		   solution, its what we have.
		*/

		Glib::signal_timeout().connect (mem_fun(*this, &ARDOUR_UI::make_session_clean), 1000);
	}

	return FALSE;
}

void
ARDOUR_UI::update_clocks ()
{
	 Clock (session->audible_frame()); /* EMIT_SIGNAL */
}

void
ARDOUR_UI::start_clocking ()
{
	clock_signal_connection = RapidScreenUpdate.connect (mem_fun(*this, &ARDOUR_UI::update_clocks));
}

void
ARDOUR_UI::stop_clocking ()
{
	clock_signal_connection.disconnect ();
}
	
void
ARDOUR_UI::toggle_clocking ()
{
#if 0
	if (clock_button.get_active()) {
		start_clocking ();
	} else {
		stop_clocking ();
	}
#endif
}

gint
ARDOUR_UI::_blink (void *arg)

{
	((ARDOUR_UI *) arg)->blink ();
	return TRUE;
}

void
ARDOUR_UI::blink ()
{
	 Blink (blink_on = !blink_on); /* EMIT_SIGNAL */
}

void
ARDOUR_UI::start_blinking ()
{
	/* Start the blink signal. Everybody with a blinking widget
	   uses Blink to drive the widget's state.
	*/

	if (blink_timeout_tag < 0) {
		blink_on = false;	
		blink_timeout_tag = gtk_timeout_add (240, _blink, this);
	}
}

void
ARDOUR_UI::stop_blinking ()
{
	if (blink_timeout_tag >= 0) {
		gtk_timeout_remove (blink_timeout_tag);
		blink_timeout_tag = -1;
	}
}


void
ARDOUR_UI::add_diskstream_to_menu (DiskStream& dstream)
{
	using namespace Gtk;
	using namespace Menu_Helpers;

	if (dstream.hidden()) {
		return;
	}

	MenuList& items = diskstream_menu->items();
	items.push_back (MenuElem (dstream.name(), bind (mem_fun(*this, &ARDOUR_UI::diskstream_selected), (gint32) dstream.id())));
}
	
void
ARDOUR_UI::diskstream_selected (gint32 id)
{
	selected_dstream = id;
	Main::quit ();
}

gint32
ARDOUR_UI::select_diskstream (GdkEventButton *ev)
{
	using namespace Gtk;
	using namespace Menu_Helpers;

	if (session == 0) {
		return -1;
	}

	diskstream_menu = new Menu();
	diskstream_menu->set_name ("ArdourContextMenu");
	using namespace Gtk;
	using namespace Menu_Helpers;

	MenuList& items = diskstream_menu->items();
	items.push_back (MenuElem (_("No Stream"), (bind (mem_fun(*this, &ARDOUR_UI::diskstream_selected), -1))));

	session->foreach_diskstream (this, &ARDOUR_UI::add_diskstream_to_menu);

	if (ev) {
		diskstream_menu->popup (ev->button, ev->time);
	} else {
		diskstream_menu->popup (0, 0);
	}

	selected_dstream = -1;

	Main::run ();

	delete diskstream_menu;

	return selected_dstream;
}

void
ARDOUR_UI::name_io_setup (AudioEngine& engine, 
			  string& buf,
			  IO& io,
			  bool in)
{
	if (in) {
		if (io.n_inputs() == 0) {
			buf = _("none");
			return;
		}
		
		/* XXX we're not handling multiple ports yet. */

		const char **connections = io.input(0)->get_connections();
		
		if (connections == 0 || connections[0] == '\0') {
			buf = _("off");
		} else {
			buf = connections[0];
		}

		free (connections);

	} else {

		if (io.n_outputs() == 0) {
			buf = _("none");
			return;
		}
		
		/* XXX we're not handling multiple ports yet. */

		const char **connections = io.output(0)->get_connections();
		
		if (connections == 0 || connections[0] == '\0') {
			buf = _("off");
		} else {
			buf = connections[0];
		}

		free (connections);
	}
}

void
ARDOUR_UI::snapshot_session ()
{
	ArdourPrompter prompter (true);
	string snapname;
	string now;
	time_t n;

	time (&n);
	now = ctime (&n);
	now = now.substr (20, 4) + now.substr (3, 16) + " (" + now.substr (0, 3) + ")";

	prompter.set_name ("Prompter");
	prompter.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	prompter.set_response_sensitive (Gtk::RESPONSE_ACCEPT, false);
	prompter.set_prompt (_("Name of New Snapshot"));
	prompter.set_initial_text (now);
	
	switch (prompter.run()) {
	case RESPONSE_ACCEPT:
		prompter.get_result (snapname);
		if (snapname.length()){
			save_state (snapname);
		}
		break;

	default:
		break;
	}
}

void
ARDOUR_UI::save_state (const string & name)
{
	(void) save_state_canfail (name);
}
		
int
ARDOUR_UI::save_state_canfail (string name)
{
	if (session) {
		int ret;

		if (name.length() == 0) {
			name = session->snap_name();
		}

		if ((ret = session->save_state (name)) != 0) {
			return ret;
		}
	}
	save_ardour_state (); /* XXX cannot fail? yeah, right ... */
	return 0;
}

void
ARDOUR_UI::restore_state (string name)
{
	if (session) {
		if (name.length() == 0) {
			name = session->name();
		}
		session->restore_state (name);
	}
}

void
ARDOUR_UI::primary_clock_value_changed ()
{
	if (session) {
		session->request_locate (primary_clock.current_time ());
	}
}

void
ARDOUR_UI::secondary_clock_value_changed ()
{
	if (session) {
		session->request_locate (secondary_clock.current_time ());
	}
}

void
ARDOUR_UI::rec_enable_button_blink (bool onoff, DiskStream *dstream, Widget *w)
{
	if (session && dstream && dstream->record_enabled()) {

		Session::RecordState rs;
		
		rs = session->record_status ();

		switch (rs) {
		case Session::Disabled:
		case Session::Enabled:
			if (w->get_state() != STATE_SELECTED) {
				w->set_state (STATE_SELECTED);
			}
			break;

		case Session::Recording:
			if (w->get_state() != STATE_ACTIVE) {
				w->set_state (STATE_ACTIVE);
			}
			break;
		}

	} else {
		if (w->get_state() != STATE_NORMAL) {
			w->set_state (STATE_NORMAL);
		}
	}
}

void
ARDOUR_UI::transport_rec_enable_blink (bool onoff) 
{
	if (session == 0) {
		return;
	}
	
	switch (session->record_status()) {
	case Session::Enabled:
		if (onoff) {
			rec_button.set_state (1);
		} else {
			rec_button.set_state (0);
		}
		break;

	case Session::Recording:
		rec_button.set_state (2);
		break;

	default:
		rec_button.set_state (0);
		break;
	}
}

gint
ARDOUR_UI::hide_and_quit (GdkEventAny *ev, ArdourDialog *window)
{
	window->hide();
	Gtk::Main::quit ();
	return TRUE;
}

void
ARDOUR_UI::start_keyboard_prefix ()
{
	keyboard->start_prefix();
}

void
ARDOUR_UI::save_template ()

{
	ArdourPrompter prompter (true);
	string name;

	prompter.set_name (X_("Prompter"));
	prompter.set_prompt (_("Name for mix template:"));
	prompter.set_initial_text(session->name() + _("-template"));
	prompter.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	prompter.set_response_sensitive (Gtk::RESPONSE_ACCEPT, false);

	switch (prompter.run()) {
	case RESPONSE_ACCEPT:
		prompter.get_result (name);
		
		if (name.length()) {
			session->save_template (name);
		}
		break;

	default:
		break;
	}
}

void
ARDOUR_UI::new_session (bool startup, std::string predetermined_path)
{
	m_new_session_dialog->show_all();
	m_new_session_dialog->set_modal(true);
	m_new_session_dialog->set_name(predetermined_path);
	m_new_session_dialog->reset_recent();

	int response = Gtk::RESPONSE_CANCEL;

	do {
		response = m_new_session_dialog->run ();
		if(response == Gtk::RESPONSE_CANCEL || response == Gtk::RESPONSE_DELETE_EVENT) {
		  quit();
		  return;

		} else if (response == 0) {
		  /* Clear was pressed */
		  m_new_session_dialog->reset();

		} else if (response == Gtk::RESPONSE_YES) {
		  /* YES  == OPEN, but there's no enum for that */
		  std::string session_name = m_new_session_dialog->session_name();
		  std::string session_path = m_new_session_dialog->session_folder();
		  load_session (session_path, session_name);


		} else if (response == Gtk::RESPONSE_OK) {
		  if (m_new_session_dialog->get_current_page() == 1) {

		    /* XXX this is a bit of a hack.. 
		       i really want the new sesion dialog to return RESPONSE_YES
		       if we're on page 1 (the load page)
		       Unfortunately i can't see how atm.. 
		    */
			std::string session_name = m_new_session_dialog->session_name();
			std::string session_path = m_new_session_dialog->session_folder();
			load_session (session_path, session_name);

		  } else {

			_session_is_new = true;
			
			std::string session_name = m_new_session_dialog->session_name();
			std::string session_path = m_new_session_dialog->session_folder();
			

			  //XXX This is needed because session constructor wants a 
			  //non-existant path. hopefully this will be fixed at some point.
			
			session_path = Glib::build_filename(session_path, session_name);
			
			std::string template_name = m_new_session_dialog->session_template_name();
			
			if (m_new_session_dialog->use_session_template()) {
				
				load_session (session_path, session_name, &template_name);
				
			} else {
				
				uint32_t cchns;
				uint32_t mchns;
				Session::AutoConnectOption iconnect;
				Session::AutoConnectOption oconnect;
				
				if (m_new_session_dialog->create_control_bus()) {
					cchns = (uint32_t) m_new_session_dialog->control_channel_count();
				} else {
					cchns = 0;
				}
				
				if (m_new_session_dialog->create_master_bus()) {
					mchns = (uint32_t) m_new_session_dialog->master_channel_count();
				} else {
					mchns = 0;
				}
				
				if (m_new_session_dialog->connect_inputs()) {
					iconnect = Session::AutoConnectPhysical;
				} else {
					iconnect = Session::AutoConnectOption (0);
				}
				
				/// @todo some minor tweaks.

				if (m_new_session_dialog->connect_outs_to_master()) {
					oconnect = Session::AutoConnectMaster;
				} else if (m_new_session_dialog->connect_outs_to_physical()) {
					oconnect = Session::AutoConnectPhysical;
				} else {
					oconnect = Session::AutoConnectOption (0);
				} 
				
				uint32_t nphysin = (uint32_t) m_new_session_dialog->input_limit_count();
				uint32_t nphysout = (uint32_t) m_new_session_dialog->output_limit_count();
				
				build_session (session_path,
					       session_name,
					       cchns,
					       mchns,
					       iconnect,
					       oconnect,
					       nphysin,
					       nphysout, 
					       engine->frame_rate() * 60 * 5);
			}
		  }	
		}
		
	} while (response == 0);
	m_new_session_dialog->hide_all();
	show();

}

void
ARDOUR_UI::close_session()
{
  unload_session();
  new_session ();
}

int
ARDOUR_UI::load_session (const string & path, const string & snap_name, string* mix_template)
{
	Session *new_session;
	int x;
	session_loaded = false;
	x = unload_session ();

	if (x < 0) {
		return -1;
	} else if (x > 0) {
		return 0;
	}

	/* if it already exists, we must have write access */

	if (::access (path.c_str(), F_OK) == 0 && ::access (path.c_str(), W_OK)) {
		MessageDialog msg (*editor, _("\
You do not have write access to this session.\n\
This prevents the session from being loaded."));
		msg.run ();
		return -1;
	}

	try {
		new_session = new Session (*engine, path, snap_name, mix_template);
	}

	catch (...) {

		error << string_compose(_("Session \"%1 (snapshot %2)\" did not load successfully"), path, snap_name) << endmsg;
		return -1;
	}

	connect_to_session (new_session);

	//if (engine->running()) {
	//mixer->show_window();
	//}
	session_loaded = true;
	return 0;
}

int
ARDOUR_UI::make_session_clean ()
{
	if (session) {
		session->set_clean ();
	}

	show ();

	return FALSE;
}

int
ARDOUR_UI::build_session (const string & path, const string & snap_name, 
			  uint32_t control_channels,
			  uint32_t master_channels, 
			  Session::AutoConnectOption input_connect,
			  Session::AutoConnectOption output_connect,
			  uint32_t nphysin,
			  uint32_t nphysout,
			  jack_nframes_t initial_length)
{
	Session *new_session;
	int x;

	session_loaded = false;
	x = unload_session ();
	if (x < 0) {
		return -1;
	} else if (x > 0) {
		return 0;
	}
	
	_session_is_new = true;

	try {
		new_session = new Session (*engine, path, snap_name, input_connect, output_connect,
					   control_channels, master_channels, nphysin, nphysout, initial_length);
	}

	catch (...) {

		error << string_compose(_("Session \"%1 (snapshot %2)\" did not load successfully"), path, snap_name) << endmsg;
		return -1;
	}

	connect_to_session (new_session);

	//if (engine->running()) {
	//mixer->show_window();
	//}
	session_loaded = true;
	return 0;
}

void
ARDOUR_UI::show ()
{
	if (editor) {
		editor->show_window ();
		shown_flag = true;
	}

	if (session && mixer) {
		// mixer->show_window ();
	}
	
	if (about) {
		about->present ();
	}
}

void
ARDOUR_UI::show_splash ()
{
	if (about == 0) {
		about = new About();
	}
	about->present();
}

void
ARDOUR_UI::hide_splash ()
{
	if (about) {
		// about->hide();
	}
}

void
ARDOUR_UI::display_cleanup_results (Session::cleanup_report& rep, const gchar* list_title, const string & msg)
{
	size_t removed;

	removed = rep.paths.size();

	if (removed == 0) {
		MessageDialog msgd (*editor,
				    _("No audio files were ready for cleanup"), 
				    true,
				    Gtk::MESSAGE_INFO,
				    (Gtk::ButtonsType)(Gtk::BUTTONS_CLOSE)  );
		msgd.set_secondary_text (_("If this seems suprising, \n\
check for any existing snapshots.\n\
These may still include regions that\n\
require some unused files to continue to exist."));
	
		msgd.run ();
		return;
	} 

	ArdourDialog results (_("ardour: cleanup"), true, false);
	
	struct CleanupResultsModelColumns : public Gtk::TreeModel::ColumnRecord {
	    CleanupResultsModelColumns() { 
		    add (visible_name);
		    add (fullpath);
	    }
	    Gtk::TreeModelColumn<Glib::ustring> visible_name;
	    Gtk::TreeModelColumn<Glib::ustring> fullpath;
	};

	
	CleanupResultsModelColumns results_columns;
	Glib::RefPtr<Gtk::ListStore> results_model;
	Gtk::TreeView results_display;
	
	results_model = ListStore::create (results_columns);
	results_display.set_model (results_model);
	results_display.append_column (list_title, results_columns.visible_name);

	results_display.set_name ("CleanupResultsList");
	results_display.set_headers_visible (true);
	results_display.set_headers_clickable (false);
	results_display.set_reorderable (false);

	Gtk::ScrolledWindow list_scroller;
	Gtk::Label txt;
	Gtk::VBox dvbox;
	Gtk::HBox dhbox;  // the hbox for the image and text
	Gtk::HBox ddhbox; // the hbox we eventually pack into the dialog's vbox
	Gtk::Image* dimage = manage (new Gtk::Image(Stock::DIALOG_INFO,  Gtk::ICON_SIZE_DIALOG));

	dimage->set_alignment(ALIGN_LEFT, ALIGN_TOP);

	if (rep.space < 1048576.0f) {
		if (removed > 1) {
		  txt.set_text (string_compose (msg, removed, _("files were"), session->path() + "dead_sounds", (float) rep.space / 1024.0f, "kilo"));
		} else {
			txt.set_text (string_compose (msg, removed, _("file was"), session->path() + "dead_sounds", (float) rep.space / 1024.0f, "kilo"));
		}
	} else {
		if (removed > 1) {
			txt.set_text (string_compose (msg, removed, _("files were"), session->path() + "dead_sounds", (float) rep.space / 1048576.0f, "mega"));
		} else {
			txt.set_text (string_compose (msg, removed, _("file was"), session->path() + "dead_sounds", (float) rep.space / 1048576.0f, "mega"));
		}
	}

	dhbox.pack_start (*dimage, true, false, 5);
	dhbox.pack_start (txt, true, false, 5);

	for (vector<string>::iterator i = rep.paths.begin(); i != rep.paths.end(); ++i) {
		TreeModel::Row row = *(results_model->append());
		row[results_columns.visible_name] = *i;
		row[results_columns.fullpath] = *i;
	}
	
	list_scroller.add (results_display);
	list_scroller.set_size_request (-1, 150);
	list_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	dvbox.pack_start (dhbox, true, false, 5);
	dvbox.pack_start (list_scroller, true, false, 5);
	ddhbox.pack_start (dvbox, true, false, 5);

	results.get_vbox()->pack_start (ddhbox, true, false, 5);
	results.add_button (Stock::CLOSE, RESPONSE_CLOSE);
	results.set_default_response (RESPONSE_CLOSE);
	results.set_position (Gtk::WIN_POS_MOUSE);
	results.show_all_children ();
	results.set_resizable (false);

	results.run ();

}

void
ARDOUR_UI::cleanup ()
{
	if (session == 0) {
		/* shouldn't happen: menu item is insensitive */
		return;
	}


	MessageDialog  checker (_("Are you sure you want to cleanup?"),
				true,
				Gtk::MESSAGE_QUESTION,
				(Gtk::ButtonsType)(Gtk::BUTTONS_NONE));

	checker.set_secondary_text(_("Cleanup is a destructive operation.\n\
ALL undo/redo information will be lost if you cleanup.\n\
After cleanup, unused audio files will be moved to a \
\"dead sounds\" location."));
	
	checker.add_button (Stock::CANCEL, RESPONSE_CANCEL);
	checker.add_button (_("Clean Up"), RESPONSE_ACCEPT);
	checker.set_default_response (RESPONSE_CANCEL);

	checker.set_name (_("CleanupDialog"));
	checker.set_wmclass (_("ardour_cleanup"), "Ardour");
	checker.set_position (Gtk::WIN_POS_MOUSE);

	switch (checker.run()) {
	case RESPONSE_ACCEPT:
		break;
	default:
		return;
	}

	Session::cleanup_report rep;

	editor->prepare_for_cleanup ();

	if (session->cleanup_sources (rep)) {
		return;
	}
	checker.hide();
	display_cleanup_results (rep, 
				 _("cleaned files"),
				 _("\
The following %1 %2 not in use and \n\
have been moved to:\n\
%3. \n\n\
Flushing the wastebasket will \n\
release an additional\n\
%4 %5bytes of disk space.\n"
					 ));
}

void
ARDOUR_UI::flush_trash ()
{
	if (session == 0) {
		/* shouldn't happen: menu item is insensitive */
		return;
	}

	Session::cleanup_report rep;

	if (session->cleanup_trash_sources (rep)) {
		return;
	}

	display_cleanup_results (rep, 
				 _("deleted file"),
				 _("The following %1 %2 deleted from\n\
%3,\n\
releasing %4 %5bytes of disk space"));
}

void
ARDOUR_UI::add_route ()
{
	int count;

	if (!session) {
		return;
	}

	if (add_route_dialog == 0) {
		add_route_dialog = new AddRouteDialog;
		editor->ensure_float (*add_route_dialog);
	}

	if (add_route_dialog->is_visible()) {
		/* we're already doing this */
		return;
	}

	ResponseType r = (ResponseType) add_route_dialog->run ();
	
	add_route_dialog->hide();

	switch (r) {
	case RESPONSE_ACCEPT:
		break;
	default:
		return;
		break;
	}

	if ((count = add_route_dialog->count()) <= 0) {
		return;
	}

	uint32_t input_chan = add_route_dialog->channels ();
	uint32_t output_chan;
	string name_template = add_route_dialog->name_template ();
	bool track = add_route_dialog->track ();

	Session::AutoConnectOption oac = session->get_output_auto_connect();

	if (oac & Session::AutoConnectMaster) {
		output_chan = (session->master_out() ? session->master_out()->n_inputs() : input_chan);
	} else {
		output_chan = input_chan;
	}

	/* XXX do something with name template */

	while (count) {
		if (track) {
			session_add_audio_track (input_chan, output_chan, add_route_dialog->mode());
		} else {
			session_add_audio_bus (input_chan, output_chan);
		}
		--count;
		
		while (Main::events_pending()) {
			Main::iteration ();
		}
	}
}

XMLNode*
ARDOUR_UI::mixer_settings () const
{
	XMLNode* node = 0;

	if (session) {
		node = session->instant_xml(X_("Mixer"), session->path());
	} else {
		node = Config->instant_xml(X_("Mixer"), get_user_ardour_path());
	}

	if (!node) {
		node = new XMLNode (X_("Mixer"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::editor_settings () const
{
	XMLNode* node = 0;

	if (session) {
		node = session->instant_xml(X_("Editor"), session->path());
	} else {
		node = Config->instant_xml(X_("Editor"), get_user_ardour_path());
	}

	if (!node) {
		node = new XMLNode (X_("Editor"));
	}
	return node;
}

XMLNode*
ARDOUR_UI::keyboard_settings () const
{
	XMLNode* node = 0;

	node = Config->extra_xml(X_("Keyboard"));
	
	if (!node) {
		node = new XMLNode (X_("Keyboard"));
	}
	return node;
}

void
ARDOUR_UI::halt_on_xrun_message ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::halt_on_xrun_message));

	MessageDialog msg (*editor,
			   _("Recording was stopped because your system could not keep up."));
	msg.run ();
}

void 
ARDOUR_UI::delete_sources_in_the_right_thread (list<ARDOUR::Source*>* deletion_list)
{
	ENSURE_GUI_THREAD (bind (mem_fun(*this, &ARDOUR_UI::delete_sources_in_the_right_thread), deletion_list));

	for (list<Source*>::iterator i = deletion_list->begin(); i != deletion_list->end(); ++i) {
		delete *i;
	}

	delete deletion_list;
}

void
ARDOUR_UI::disk_overrun_handler ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::disk_underrun_handler));

	if (!have_disk_overrun_displayed) {
		have_disk_overrun_displayed = true;
		MessageDialog msg (*editor, X_("diskrate dialog"), _("\
The disk system on your computer\n\
was not able to keep up with Ardour.\n\
\n\
Specifically, it failed to write data to disk\n\
quickly enough to keep up with recording.\n"));
		msg.run ();
		have_disk_overrun_displayed = false;
	}
}

void
ARDOUR_UI::disk_underrun_handler ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &ARDOUR_UI::disk_underrun_handler));

	if (!have_disk_underrun_displayed) {
		have_disk_underrun_displayed = true;
		MessageDialog msg (*editor,
			(_("The disk system on your computer\n\
was not able to keep up with Ardour.\n\
\n\
Specifically, it failed to read data from disk\n\
quickly enough to keep up with playback.\n")));
		msg.run ();
		have_disk_underrun_displayed = false;
	} 
}

void
ARDOUR_UI::disk_underrun_message_gone ()
{
	have_disk_underrun_displayed = false;
}

void
ARDOUR_UI::disk_overrun_message_gone ()
{
	have_disk_underrun_displayed = false;
}

int
ARDOUR_UI::pending_state_dialog ()
{
	ArdourDialog dialog ("pending state dialog");
	Label  message (_("\
This session appears to have been in\n\
middle of recording when ardour or\n\
the computer was shutdown.\n\
\n\
Ardour can recover any captured audio for\n\
you, or it can ignore it. Please decide\n\
what you would like to do.\n"));

	dialog.get_vbox()->pack_start (message);
	dialog.add_button (_("Recover from crash"), RESPONSE_ACCEPT);
	dialog.add_button (_("Ignore crash data"), RESPONSE_REJECT);

	dialog.set_position (WIN_POS_CENTER);
	dialog.show_all ();
	
	switch (dialog.run ()) {
	case RESPONSE_ACCEPT:
		return 1;
	default:
		return 0;
	}
}
	
void
ARDOUR_UI::disconnect_from_jack ()
{
	if (engine) {
		if( engine->disconnect_from_jack ()) {
			MessageDialog msg (*editor, _("Could not disconnect from JACK"));
			msg.run ();
		}

		update_sample_rate (0);
	}
}

void
ARDOUR_UI::reconnect_to_jack ()
{
	if (engine) {
		if (engine->reconnect_to_jack ()) {
			MessageDialog msg (*editor,  _("Could not reconnect to JACK"));
			msg.run ();
		}

		update_sample_rate (0);
	}
}

void
ARDOUR_UI::set_jack_buffer_size (jack_nframes_t nframes)
{
	engine->request_buffer_size (nframes);
	update_sample_rate (0);
}

int
ARDOUR_UI::cmdline_new_session (string path)
{
	if (path[0] != '/') {
		char buf[PATH_MAX+1];
		string str;

		getcwd (buf, sizeof (buf));
		str = buf;
		str += '/';
		str += path;
		path = str;
	}

	new_session (false, path);

	_will_create_new_session_automatically = false; /* done it */
	return FALSE; /* don't call it again */
}

void
ARDOUR_UI::set_native_file_header_format (HeaderFormat hf)
{
	Glib::RefPtr<Action> act;
	
	switch (hf) {
	case BWF:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatBWF"));
		break;
	case WAVE:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatWAVE"));
		break;
	case WAVE64:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatWAVE64"));
		break;
	case iXML:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatiXML"));
		break;
	case RF64:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatRF64"));
		break;
	}

	if (act) {
		Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && ract->get_active() && Config->get_native_file_header_format() != hf) {
			Config->set_native_file_header_format (hf);
			if (session) {
				session->reset_native_file_format ();
			}
		}
	}
}

void
ARDOUR_UI::set_native_file_data_format (SampleFormat sf)
{
	Glib::RefPtr<Action> act;
	
	switch (sf) {
	case FormatFloat:
		act = ActionManager::get_action (X_("options"), X_("FileDataFormatFloat"));
		break;
	case FormatInt24:
		act = ActionManager::get_action (X_("options"), X_("FileDataFormat24bit"));
		break;
	}

	if (act) {
		Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic(act);

		if (ract && ract->get_active() && Config->get_native_file_data_format() != sf) {
			Config->set_native_file_data_format (sf);
			if (session) {
				session->reset_native_file_format ();
			}
		}
	}
}

void
ARDOUR_UI::use_config ()
{
	Glib::RefPtr<Action> act;

	switch (Config->get_native_file_data_format ()) {
	case FormatFloat:
		act = ActionManager::get_action (X_("options"), X_("FileDataFormatFloat"));
		break;
	case FormatInt24:
		act = ActionManager::get_action (X_("options"), X_("FileDataFormat24bit"));
		break;
	}

	if (act) {
		Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic(act);
		ract->set_active ();
	}	

	switch (Config->get_native_file_header_format ()) {
	case BWF:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatBWF"));
		break;
	case WAVE:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatWAVE"));
		break;
	case WAVE64:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatWAVE64"));
		break;
	case iXML:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatiXML"));
		break;
	case RF64:
		act = ActionManager::get_action (X_("options"), X_("FileHeaderFormatRF64"));
		break;
	}

	if (act) {
		Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic(act);
		ract->set_active ();
	}	
}
