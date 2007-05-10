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

#ifndef __gtk_ardour_add_route_dialog_h__
#define __gtk_ardour_add_route_dialog_h__

#include <string>

#include <gtkmm/entry.h>
#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>

#include <ardour/types.h>
#include <ardour/data_type.h>

class AddRouteDialog : public Gtk::Dialog
{
  public:
	AddRouteDialog ();
	~AddRouteDialog ();

	bool track ();
	ARDOUR::DataType type();
	std::string name_template ();
	int channels ();
	int count ();
	ARDOUR::TrackMode mode();

  private:
	Gtk::Entry name_template_entry;
	Gtk::RadioButton track_button;
	Gtk::RadioButton bus_button;
	Gtk::Adjustment routes_adjustment;
	Gtk::SpinButton routes_spinner;
	Gtk::ComboBoxText channel_combo;
	Gtk::ComboBoxText track_mode_combo;
	Gtk::Frame aframe;
	Gtk::Frame ccframe;

	void track_type_chosen ();
};

#endif /* __gtk_ardour_add_route_dialog_h__ */
