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

#ifndef __ardour_gtk_color_manager_h__
#define __ardour_gtk_color_manager_h__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/colorselection.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/rc.h>
#include "ardour_dialog.h"
#include "ui_config.h"

class ThemeManager : public ArdourDialog
{
  public:
	ThemeManager();
	~ThemeManager();

	int save (std::string path);
	void setup_theme ();
	
	void on_dark_theme_button_toggled ();
	void on_light_theme_button_toggled ();

  private:
	struct ColorDisplayModelColumns : public Gtk::TreeModel::ColumnRecord {
	    ColorDisplayModelColumns() { 
		    add (name);
		    add (color);
		    add (gdkcolor);
			add (pVar);
		    add (rgba);
	    }
	    
	    Gtk::TreeModelColumn<Glib::ustring>  name;
	    Gtk::TreeModelColumn<Glib::ustring>  color;
	    Gtk::TreeModelColumn<Gdk::Color>     gdkcolor;
	    Gtk::TreeModelColumn<UIConfigVariable<uint32_t> *> pVar;
	    Gtk::TreeModelColumn<uint32_t>       rgba;
	};

	ColorDisplayModelColumns columns;
	Gtk::TreeView color_display;
	Glib::RefPtr<Gtk::ListStore> color_list;
	Gtk::ColorSelectionDialog color_dialog;
	Gtk::ScrolledWindow scroller;
	Gtk::HBox theme_selection_hbox;
	Gtk::RadioButton dark_button;
	Gtk::RadioButton light_button;

	bool button_press_event (GdkEventButton*);
};

#endif /* __ardour_gtk_color_manager_h__ */

