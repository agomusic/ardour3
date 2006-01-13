/*
    Copyright (C) 1999 Paul Barton-Davis 

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

#include <string>

#include <pbd/whitespace.h>

#include <gtkmm/stock.h>
#include <gtkmm2ext/prompter.h>

#include "i18n.h"

using namespace std;
using namespace Gtkmm2ext;

Prompter::Prompter (Gtk::Window& parent, bool modal)
	: Gtk::Dialog ("", parent, modal)
{
	init ();
}

Prompter::Prompter (bool modal)
	: Gtk::Dialog ("", modal)
{
	init ();
}

void
Prompter::init ()
{
	set_position (Gtk::WIN_POS_MOUSE);
	set_name ("Prompter");

	set_default_response (Gtk::RESPONSE_ACCEPT);
	
	add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	add_button (Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	
	entryLabel.set_line_wrap (true);
	entryLabel.set_name ("PrompterLabel");

	entryBox.set_homogeneous (false);
	entryBox.set_spacing (5);
	entryBox.set_border_width (10);
	entryBox.pack_start (entryLabel);
	entryBox.pack_start (entry, false, false);

	get_vbox()->pack_start (entryBox);
	show_all_children();

	entry.signal_activate().connect (bind (mem_fun (*this, &Prompter::response), Gtk::RESPONSE_ACCEPT));
}	

void
Prompter::change_labels (string okstr, string cancelstr)
{
	// dynamic_cast<Gtk::Label*>(ok.get_child())->set_text (okstr);
	// dynamic_cast<Gtk::Label*>(cancel.get_child())->set_text (cancelstr);
}

void
Prompter::get_result (string &str, bool strip)
{
	str = entry.get_text ();
	if (strip) {
		strip_whitespace_edges (str);
	}
}
