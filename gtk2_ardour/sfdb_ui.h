/*
    Copyright (C) 2005-2006 Paul Davis

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

#ifndef __ardour_sfdb_ui_h__
#define __ardour_sfdb_ui_h__

#include <string>
#include <vector>

#include <sigc++/signal.h>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>

#include <ardour/session.h>
#include <ardour/audiofilesource.h>

#include "ardour_dialog.h"
#include "editing.h"

class SoundFileBox : public Gtk::VBox
{
  public:
	SoundFileBox ();
	virtual ~SoundFileBox () {};
	
	void set_session (ARDOUR::Session* s);
	bool setup_labels (std::string filename);

  protected:
	ARDOUR::Session* _session;
	std::string path;
	
	ARDOUR::SoundFileInfo sf_info;
	
	pid_t current_pid;
	
	Gtk::Label length;
	Gtk::Label format;
	Gtk::Label channels;
	Gtk::Label samplerate;
	Gtk::Label timecode;
	
	Gtk::Frame border_frame;
	
	Gtk::Entry tags_entry;
	
	Gtk::VBox main_box;
	Gtk::VBox path_box;
	Gtk::HBox bottom_box;
	
	Gtk::Button play_btn;
	Gtk::Button stop_btn;
	Gtk::Button apply_btn;
	
	bool tags_entry_left (GdkEventFocus* event);
	void play_btn_clicked ();
	void stop_btn_clicked ();
	void apply_btn_clicked ();
	
	void audition_status_changed (bool state);
};

class SoundFileBrowser : public ArdourDialog
{
  public:
	SoundFileBrowser (std::string title, ARDOUR::Session* _s = 0);
	virtual ~SoundFileBrowser () {}; 
	
	virtual void set_session (ARDOUR::Session*);

  protected:
	Gtk::FileChooserWidget chooser;
	Gtk::FileFilter filter;
	SoundFileBox preview;
	
	class FoundTagColumns : public Gtk::TreeModel::ColumnRecord
	{
	  public:
		Gtk::TreeModelColumn<string> pathname;
		
		FoundTagColumns() { add(pathname); }
	};
	
	FoundTagColumns found_list_columns;
	Glib::RefPtr<Gtk::ListStore> found_list;
	Gtk::TreeView found_list_view;
	Gtk::Entry found_entry;
	Gtk::Button found_search_btn;
	
	Gtk::Notebook notebook;
	
	void update_preview ();
	void found_list_view_selected ();
	void found_search_clicked ();
	
	bool on_custom (const Gtk::FileFilter::Info& filter_info);
};

class SoundFileChooser : public SoundFileBrowser
{
  public:
	SoundFileChooser (std::string title, ARDOUR::Session* _s = 0);
	virtual ~SoundFileChooser () {};
	
	std::string get_filename () {return chooser.get_filename();};
};

class SoundFileOmega : public SoundFileBrowser
{
  public:
	SoundFileOmega (std::string title, ARDOUR::Session* _s);
	virtual ~SoundFileOmega () {};
	
	/* these are returned by the Dialog::run() method. note
	   that builtin GTK responses are all negative, leaving
	   positive values for application-defined responses.
	*/
	
	const static int ResponseImport = 1;
	const static int ResponseEmbed = 2;
	
	std::vector<Glib::ustring> get_paths ();
	bool get_split ();
	
	void set_mode (Editing::ImportMode);
	Editing::ImportMode get_mode ();

  protected:
	Gtk::CheckButton  split_check;
	Gtk::ComboBoxText mode_combo;
	
	void mode_changed ();
	
	static std::vector<std::string> mode_strings;
};

#endif // __ardour_sfdb_ui_h__
