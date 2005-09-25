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

*/

#ifndef __ardour_export_dialog_h__
#define __ardour_export_dialog_h__

#include <gtk--.h>

#include <ardour/export.h>
#include "ardour_dialog.h"
#include <ardour/location.h>


class PublicEditor;

namespace ARDOUR {
	class Session;
	class AudioRegion;
}

class ExportDialog : public ArdourDialog
{
  public:
	ExportDialog (PublicEditor&, ARDOUR::AudioRegion* r = 0);
	~ExportDialog ();

	void connect_to_session (ARDOUR::Session*);
	void set_range (jack_nframes_t start, jack_nframes_t end);
	void start_export ();

  private:
	PublicEditor&    editor;
	ARDOUR::Session* session;
	ARDOUR::AudioRegion* audio_region;
	Gtk::VBox   vpacker;
	Gtk::VBox   track_vpacker;
	Gtk::HBox   hpacker;
	Gtk::HBox   button_box;

	Gtk::Table  format_table;
	Gtk::Frame  format_frame;

	Gtk::Label  sample_rate_label;
	Gtk::Combo  sample_rate_combo;
	Gtk::Label  src_quality_label;
	Gtk::Combo  src_quality_combo;
	Gtk::Label  dither_type_label;
	Gtk::Combo  dither_type_combo;
	Gtk::Label  cue_file_label;
	Gtk::Combo  cue_file_combo;
	Gtk::Label  channel_count_label;
	Gtk::Combo  channel_count_combo;
	Gtk::Label  header_format_label;
	Gtk::Combo  header_format_combo;
	Gtk::Label  bitdepth_format_label;
	Gtk::Combo  bitdepth_format_combo;
	Gtk::Label  endian_format_label;
	Gtk::Combo  endian_format_combo;
	Gtk::CheckButton cuefile_only_checkbox;

	Gtk::Frame  file_frame;
	Gtk::Entry  file_entry;
	Gtk::HBox   file_hbox;
	Gtk::Button file_browse_button;

	Gtk::Button ok_button;
	Gtk::Button cancel_button;
	Gtk::Label  cancel_label;
	Gtk::ProgressBar progress_bar;
	Gtk::ScrolledWindow track_scroll;
	Gtk::ScrolledWindow master_scroll;
	Gtk::Button         track_selector_button;
	Gtk::CList  track_selector;
	Gtk::CList  master_selector;
	Gtk::FileSelection *file_selector;
	ARDOUR::AudioExportSpecification spec;

	static GdkPixmap *check_pixmap;
	static GdkBitmap *check_mask;
	static GdkPixmap *empty_pixmap;
	static GdkBitmap *empty_mask;

	static void *_thread (void *arg);
	gint progress_timeout ();
	SigC::Connection progress_connection;
	void build_window ();
	void end_dialog();
	gint header_chosen (GdkEventAny *ignored);
	gint channels_chosen (GdkEventAny *ignored);
	gint bitdepth_chosen (GdkEventAny *ignored);
	gint sample_rate_chosen (GdkEventAny *ignored);
	gint cue_file_type_chosen(GdkEventAny *ignored);
	gint track_selector_button_press_event (GdkEventButton *ev);
	gint master_selector_button_press_event (GdkEventButton *ev);

      	void do_export_cd_markers (const string& path, const string& cuefile_type);
	void export_cue_file (ARDOUR::Locations::LocationList& locations, const string& path);
	void export_toc_file (ARDOUR::Locations::LocationList& locations, const string& path);
	void do_export ();
	gint change_focus_policy (GdkEventAny *, bool);
	gint window_closed (GdkEventAny *ignored);

	void track_selector_button_click ();

	void initiate_browse ();
	void finish_browse (int status);

	void set_state();
	void save_state();

	static void* _export_region_thread (void *);
	void export_region ();
};

#endif // __ardour_export_dialog_h__

