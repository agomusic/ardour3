/*
    Copyright (C) 2005 Paul Davis 
    Written by Taybin Rutkin

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

#include <sndfile.h>

#include <sigc++/signal.h>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>

#include <ardour/session.h>

#include "ardour_dialog.h"

class SoundFileBox : public Gtk::VBox
{
  public:
    SoundFileBox (ARDOUR::Session* session);
    virtual ~SoundFileBox () {};

    bool update (std::string filename);

  protected:
    struct LabelModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
      Gtk::TreeModelColumn<std::string> field;
      Gtk::TreeModelColumn<std::string> data;

      LabelModelColumns() { add(field); add(data); }
    };

    LabelModelColumns label_columns;
    
    SF_INFO sf_info;

    pid_t current_pid;

    Gtk::Label label;
    Gtk::Label path;
    Gtk::Entry path_entry;
    Gtk::Label length;
    Gtk::Label format;
    Gtk::Label channels;
    Gtk::Label samplerate;

    Gtk::TreeView field_view;
    Glib::RefPtr<Gtk::ListStore> fields;
    std::string selected_field;

    Gtk::Frame border_frame;

    Gtk::VBox main_box;
    Gtk::VBox path_box;
    Gtk::HBox top_box;
    Gtk::HBox bottom_box;

    Gtk::Button play_btn;
    Gtk::Button stop_btn;
    Gtk::Button add_field_btn;
    Gtk::Button remove_field_btn;

 //   void fields_refiller (Gtk::CList &clist);
    int setup_labels (std::string filename);
    void setup_fields ();

    void play_btn_clicked ();
    void stop_btn_clicked ();
    void add_field_clicked ();
    void remove_field_clicked ();

    void field_selected ();
    void audition_status_changed (bool state);
};

class SoundFileBrowser : public ArdourDialog
{
  public:
    SoundFileBrowser (std::string title);
    virtual ~SoundFileBrowser () {}; 

  protected:
    Gtk::FileChooserWidget chooser;
    SoundFileBox preview;

    void update_preview ();
};

class SoundFileChooser : public SoundFileBrowser
{
  public:
    SoundFileChooser (std::string title);
    virtual ~SoundFileChooser () {};

    std::string get_filename () {return chooser.get_filename();};
};

class SoundFileOmega : public SoundFileBrowser
{
  public:
    SoundFileOmega (std::string title);
    virtual ~SoundFileOmega () {};

    sigc::signal<void, std::vector<std::string>, bool> Embedded;
    sigc::signal<void, std::vector<std::string>, bool> Imported;

  protected:
    Gtk::Button embed_btn;
    Gtk::Button import_btn;
    Gtk::CheckButton split_check;

    void embed_clicked ();
    void import_clicked ();
};

#endif // __ardour_sfdb_ui_h__
