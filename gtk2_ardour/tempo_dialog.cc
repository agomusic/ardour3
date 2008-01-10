/*
    Copyright (C) 2000-2007 Paul Davis 

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

#include <cstdio> // for snprintf, grrr 

#include <gtkmm/stock.h>
#include <gtkmm2ext/utils.h>

#include "tempo_dialog.h"
#include "utils.h"

#include "i18n.h"

using namespace Gtk;
using namespace Gtkmm2ext;
using namespace ARDOUR;
using namespace PBD;

TempoDialog::TempoDialog (TempoMap& map, nframes_t frame, const string & action)
	: ArdourDialog (_("edit tempo")),
	  bpm_adjustment (60.0, 1.0, 999.9, 0.1, 1.0, 1.0),
	  bpm_spinner (bpm_adjustment),
	  bpm_frame (_("Beats per minute")),
	  note_frame (_("BPM denominator")),
	  ok_button (action),
	  cancel_button (_("Cancel")),
	  when_bar_label (_("Bar")),
	  when_beat_label (_("Beat")),
	  when_table (2, 2),
	  when_frame (_("Location"))
{
	BBT_Time when;
	Tempo tempo (map.tempo_at (frame));
	map.bbt_time (frame, when);

	init (when, tempo.beats_per_minute(), tempo.note_type(), true);
}

TempoDialog::TempoDialog (TempoSection& section, const string & action)
	: ArdourDialog ("tempo dialog"),
	  bpm_adjustment (60.0, 1.0, 999.9, 0.1, 1.0, 1.0),
	  bpm_spinner (bpm_adjustment),
	  bpm_frame (_("Beats per minute")),
	  ok_button (action),
	  cancel_button (_("Cancel")),
	  when_bar_label (_("Bar")),
	  when_beat_label (_("Beat")),
	  when_table (2, 2),
	  when_frame (_("Location"))
{
	init (section.start(), section.beats_per_minute(), section.note_type(), section.movable());
}

void
TempoDialog::init (const BBT_Time& when, double bpm, double note_type, bool movable)
{
	bpm_spinner.set_numeric (true);
	bpm_spinner.set_digits (2);
	bpm_spinner.set_wrap (true);
	bpm_spinner.set_value (bpm);

	strings.push_back (_("whole (1)"));
	strings.push_back (_("second (2)"));
	strings.push_back (_("third (3)"));
	strings.push_back (_("quarter (4)"));
	strings.push_back (_("eighth (8)"));
	strings.push_back (_("sixteenth (16)"));
	strings.push_back (_("thirty-second (32)"));
	
	/* the string here needs to be the longest one to display */
	const guint32 FUDGE = 20; // Combo's are stupid - they steal space from the entry for the button
	// TRANSLATORS: this is not a mis-spelling of "thirty", we're including a vertical 
	// descender to make sure the height gets computed properly.
        Gtkmm2ext::set_size_request_to_display_given_text (note_types, "thirtq-second (32)", 7+FUDGE, 15);

	set_popdown_strings (note_types, strings);

	if (note_type==1.0f)
		note_types.set_active_text (_("whole (1)"));
	else if (note_type==2.0f)
		note_types.set_active_text (_("second (2)"));
	else if (note_type==3.0f)
		note_types.set_active_text (_("third (3)"));
	else if (note_type==4.0f)
		note_types.set_active_text (_("quarter (4)"));
	else if (note_type==8.0f)
		note_types.set_active_text (_("eighth (8)"));
	else if (note_type==16.0f)
		note_types.set_active_text (_("sixteenth (16)"));
	else if (note_type==32.0f)
		note_types.set_active_text (_("thirty-second (32)"));
	else
		note_types.set_active_text (_("quarter (4)"));

	hspacer1.set_border_width (5);
	hspacer1.pack_start (bpm_spinner, false, false);
	vspacer1.set_border_width (5);
	vspacer1.pack_start (hspacer1, false, false);

	hspacer2.set_border_width (5);
	hspacer2.pack_start (note_types, false, false);
	vspacer2.set_border_width (5);
	vspacer2.pack_start (hspacer2, false, false);

	bpm_frame.add (vspacer1);
	note_frame.add (vspacer2);

	if (movable) {
		snprintf (buf, sizeof (buf), "%" PRIu32, when.bars);
		when_bar_entry.set_text (buf);
		snprintf (buf, sizeof (buf), "%" PRIu32, when.beats);
		when_beat_entry.set_text (buf);
		
		when_bar_entry.set_name ("MetricEntry");
		when_beat_entry.set_name ("MetricEntry");
		
		when_bar_label.set_name ("MetricLabel");
		when_beat_label.set_name ("MetricLabel");
		
		Gtkmm2ext::set_size_request_to_display_given_text (when_bar_entry, "999g", 5, 7);
		Gtkmm2ext::set_size_request_to_display_given_text (when_beat_entry, "999g", 5, 7);
		
		when_table.set_homogeneous (true);
		when_table.set_row_spacings (2);
		when_table.set_col_spacings (2);
		when_table.set_border_width (5);
		
		when_table.attach (when_bar_label, 0, 1, 0, 1, Gtk::AttachOptions(0), Gtk::FILL|Gtk::EXPAND);
		when_table.attach (when_bar_entry, 0, 1, 1, 2, Gtk::AttachOptions(0), Gtk::FILL|Gtk::EXPAND);
		
		when_table.attach (when_beat_label, 1, 2, 0, 1, Gtk::AttachOptions(0), Gtk::AttachOptions(0));
		when_table.attach (when_beat_entry, 1, 2, 1, 2, Gtk::AttachOptions(0), Gtk::AttachOptions(0));
		
		when_frame.set_name ("MetricDialogFrame");
		when_frame.add (when_table);

		get_vbox()->pack_start (when_frame, false, false);
	}

	bpm_frame.set_name ("MetricDialogFrame");
	bpm_spinner.set_name ("MetricEntry");
	note_frame.set_name ("MetricDialogFrame");

	get_vbox()->set_border_width (12);
	get_vbox()->pack_start (bpm_frame, false, false);
	get_vbox()->pack_start (note_frame, false, false);

	add_button (Stock::CANCEL, RESPONSE_CANCEL);
	add_button (Stock::APPLY, RESPONSE_ACCEPT);
	set_response_sensitive (Gtk::RESPONSE_ACCEPT, false);
	set_default_response (RESPONSE_ACCEPT);

	get_vbox()->show_all();
	bpm_spinner.show();

	set_name ("MetricDialog");

	bpm_spinner.signal_activate().connect (bind (mem_fun (*this, &TempoDialog::response), RESPONSE_ACCEPT));
	bpm_spinner.signal_button_press_event().connect (mem_fun (*this, &TempoDialog::bpm_button_press), false);
	bpm_spinner.signal_button_release_event().connect (mem_fun (*this, &TempoDialog::bpm_button_release), false);
	note_types.signal_changed().connect (mem_fun (*this, &TempoDialog::note_types_change));
}

bool
TempoDialog::bpm_button_press (GdkEventButton* ev)
{
	return false;
}

bool
TempoDialog::bpm_button_release (GdkEventButton* ev)
{	
	/* the value has been modified, accept should work now */

	set_response_sensitive (Gtk::RESPONSE_ACCEPT, true);
	return false;
}

double 
TempoDialog::get_bpm ()
{
	return bpm_spinner.get_value ();
}	

bool
TempoDialog::get_bbt_time (BBT_Time& requested)
{
	if (sscanf (when_bar_entry.get_text().c_str(), "%" PRIu32, &requested.bars) != 1) {
		return false;
	}
	
	if (sscanf (when_beat_entry.get_text().c_str(), "%" PRIu32, &requested.beats) != 1) {
		return false;
	}

	return true;
}

double
TempoDialog::get_note_type ()
{
	double note_type = 0;
	vector<string>::iterator i;
	string text = note_types.get_active_text();
	
	for (i = strings.begin(); i != strings.end(); ++i) {
		if (text == *i) {
			if (sscanf (text.c_str(), "%*[^0-9]%lf", &note_type) != 1) {
				error << string_compose(_("garbaged note type entry (%1)"), text) << endmsg;
				return 0;
			} else {
				break;
			}
		}
	} 
	
	if (i == strings.end()) {
		if (sscanf (text.c_str(), "%lf", &note_type) != 1) {
			error << string_compose(_("incomprehensible note type entry (%1)"), text) << endmsg;
			return 0;
		}
	}

	cerr << "returning " << note_type << " based on " << text << endl;
	return note_type;
}

void
TempoDialog::note_types_change ()
{
        set_response_sensitive (Gtk::RESPONSE_ACCEPT, true);
}


MeterDialog::MeterDialog (TempoMap& map, nframes_t frame, const string & action)
	: ArdourDialog ("meter dialog"),
	  note_frame (_("Meter denominator")),
	  bpb_frame (_("Beats per bar")),
	  ok_button (action),
	  cancel_button (_("Cancel")),
	  when_bar_label (_("Bar")),
	  when_beat_label (_("Beat")),
	  when_frame (_("Location"))
{
	BBT_Time when;
	frame = map.round_to_bar(frame,0); 
	Meter meter (map.meter_at(frame));

	map.bbt_time (frame, when);
	init (when, meter.beats_per_bar(), meter.note_divisor(), true);
}

MeterDialog::MeterDialog (MeterSection& section, const string & action)
	: ArdourDialog ("meter dialog"),
	  note_frame (_("Meter denominator")),
	  bpb_frame (_("Beats per bar")),
	  ok_button (action),
	  cancel_button (_("Cancel")),
	  when_bar_label (_("Bar")),
	  when_beat_label (_("Beat")),
	  when_frame (_("Location"))
{
	init (section.start(), section.beats_per_bar(), section.note_divisor(), section.movable());
}

void
MeterDialog::init (const BBT_Time& when, double bpb, double note_type, bool movable)
{
	snprintf (buf, sizeof (buf), "%.2f", bpb);
	bpb_entry.set_text (buf);
	bpb_entry.select_region (0, -1);
	Gtkmm2ext::set_size_request_to_display_given_text (bpb_entry, "999999g", 5, 5);

	strings.push_back (_("whole (1)"));
	strings.push_back (_("second (2)"));
	strings.push_back (_("third (3)"));
	strings.push_back (_("quarter (4)"));
	strings.push_back (_("eighth (8)"));
	strings.push_back (_("sixteenth (16)"));
	strings.push_back (_("thirty-second (32)"));
	
	/* the string here needs to be the longest one to display */
	const guint32 FUDGE = 20; // Combo's are stupid - they steal space from the entry for the button

	// TRANSLATORS: this is not a mis-spelling of "thirty", we're including a vertical 
	// descender to make sure the height gets computed properly.
        Gtkmm2ext::set_size_request_to_display_given_text (note_types, _("thirtq-second (32)"), 7+FUDGE, 15);

	set_popdown_strings (note_types, strings);

	if (note_type==1.0f)
		note_types.set_active_text (_("whole (1)"));
	else if (note_type==2.0f)
		note_types.set_active_text (_("second (2)"));
	else if (note_type==3.0f)
		note_types.set_active_text (_("third (3)"));
	else if (note_type==4.0f)
		note_types.set_active_text (_("quarter (4)"));
	else if (note_type==8.0f)
		note_types.set_active_text (_("eighth (8)"));
	else if (note_type==16.0f)
		note_types.set_active_text (_("sixteenth (16)"));
	else if (note_type==32.0f)
		note_types.set_active_text (_("thirty-second (32)"));
	else
		note_types.set_active_text (_("quarter (4)"));
		
	hspacer1.set_border_width (5);
	hspacer1.pack_start (note_types, false, false);
	vspacer1.set_border_width (5);
	vspacer1.pack_start (hspacer1, false, false);

	hspacer2.set_border_width (5);
	hspacer2.pack_start (bpb_entry, false, false);
	vspacer2.set_border_width (5);
	vspacer2.pack_start (hspacer2, false, false);

	note_frame.add (vspacer1);
	bpb_frame.add (vspacer2);

	if (movable) {
		snprintf (buf, sizeof (buf), "%" PRIu32, when.bars);
		when_bar_entry.set_text (buf);
		snprintf (buf, sizeof (buf), "%" PRIu32, when.beats);
		when_beat_entry.set_text (buf);
		
		when_bar_entry.set_name ("MetricEntry");
		when_beat_entry.set_name ("MetricEntry");
		
		when_bar_label.set_name ("MetricLabel");
		when_beat_label.set_name ("MetricLabel");
		
		Gtkmm2ext::set_size_request_to_display_given_text (when_bar_entry, "999g", 5, 7);
		Gtkmm2ext::set_size_request_to_display_given_text (when_beat_entry, "999g", 5, 7);
		
		when_table.set_homogeneous (true);
		when_table.set_row_spacings (2);
		when_table.set_col_spacings (2);
		when_table.set_border_width (5);
		
		when_table.attach (when_bar_label, 0, 1, 0, 1, Gtk::AttachOptions(0), Gtk::FILL|Gtk::EXPAND);
		when_table.attach (when_bar_entry, 0, 1, 1, 2, Gtk::AttachOptions(0), Gtk::FILL|Gtk::EXPAND);
		
		when_table.attach (when_beat_label, 1, 2, 0, 1, Gtk::AttachOptions(0), Gtk::AttachOptions(0));
		when_table.attach (when_beat_entry, 1, 2, 1, 2, Gtk::AttachOptions(0), Gtk::AttachOptions(0));
		
		when_frame.set_name ("MetricDialogFrame");
		when_frame.add (when_table);
		
		get_vbox()->pack_start (when_frame, false, false);
	}

	get_vbox()->set_border_width (12);
	get_vbox()->pack_start (bpb_frame, false, false);
	get_vbox()->pack_start (note_frame, false, false);
	
	bpb_frame.set_name ("MetricDialogFrame");
	note_frame.set_name ("MetricDialogFrame");
	bpb_entry.set_name ("MetricEntry");

	add_button (Stock::CANCEL, RESPONSE_CANCEL);
	add_button (Stock::APPLY, RESPONSE_ACCEPT);
	set_response_sensitive (RESPONSE_ACCEPT, false);
	set_default_response (RESPONSE_ACCEPT);

	get_vbox()->show_all ();
	bpb_entry.show ();

	set_name ("MetricDialog");
	bpb_entry.signal_activate().connect (bind (mem_fun (*this, &MeterDialog::response), RESPONSE_ACCEPT));
	bpb_entry.signal_key_press_event().connect (mem_fun (*this, &MeterDialog::bpb_key_press), false);
	bpb_entry.signal_key_release_event().connect (mem_fun (*this, &MeterDialog::bpb_key_release));
	note_types.signal_changed().connect (mem_fun (*this, &MeterDialog::note_types_change));
}

bool
MeterDialog::bpb_key_press (GdkEventKey* ev)
{

switch (ev->keyval) { 

 case GDK_0:
 case GDK_1:
 case GDK_2:
 case GDK_3:
 case GDK_4:
 case GDK_5:
 case GDK_6:
 case GDK_7:
 case GDK_8:
 case GDK_9:
 case GDK_KP_0:
 case GDK_KP_1:
 case GDK_KP_2:
 case GDK_KP_3:
 case GDK_KP_4:
 case GDK_KP_5:
 case GDK_KP_6:
 case GDK_KP_7:
 case GDK_KP_8:
 case GDK_KP_9:
 case GDK_period:
 case GDK_comma:
 case  GDK_KP_Delete:
 case  GDK_KP_Enter:
 case  GDK_Delete:
 case  GDK_BackSpace:
 case  GDK_Escape:
 case  GDK_Return:
 case  GDK_Home:
 case  GDK_End:
 case  GDK_Left:
 case  GDK_Right:
 case  GDK_Num_Lock:
 case  GDK_Tab:
    return FALSE;
 default:
      break;
 }

   return TRUE;
}

bool
MeterDialog::bpb_key_release (GdkEventKey* ev)
{
        if (bpb_entry.get_text() != "") {
	        set_response_sensitive (RESPONSE_ACCEPT, true);
	} else {
	        set_response_sensitive (RESPONSE_ACCEPT, false);
	}
	return false;
}

void
MeterDialog::note_types_change ()
{
        set_response_sensitive (Gtk::RESPONSE_ACCEPT, true);
}

double
MeterDialog::get_bpb ()
{
	double bpb = 0;
	
	if (sscanf (bpb_entry.get_text().c_str(), "%lf", &bpb) != 1) {
		return 0;
	}

	return bpb;
}
	
double
MeterDialog::get_note_type ()
{
	double note_type = 0;
	vector<string>::iterator i;
	string text = note_types.get_active_text();
	
	for (i = strings.begin(); i != strings.end(); ++i) {
		if (text == *i) {
			if (sscanf (text.c_str(), "%*[^0-9]%lf", &note_type) != 1) {
				error << string_compose(_("garbaged note type entry (%1)"), text) << endmsg;
				return 0;
			} else {
				break;
			}
		}
	} 
	
	if (i == strings.end()) {
		if (sscanf (text.c_str(), "%lf", &note_type) != 1) {
			error << string_compose(_("incomprehensible note type entry (%1)"), text) << endmsg;
			return 0;
		}
	}

	return note_type;
}

bool
MeterDialog::get_bbt_time (BBT_Time& requested)
{
	requested.ticks = 0;

	if (sscanf (when_bar_entry.get_text().c_str(), "%" PRIu32, &requested.bars) != 1) {
		return false;
	}
	
	if (sscanf (when_beat_entry.get_text().c_str(), "%" PRIu32, &requested.beats) != 1) {
		return false;
	}

	return true;
}
