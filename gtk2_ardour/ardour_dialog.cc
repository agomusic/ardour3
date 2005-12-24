/*
    Copyright (C) 2002 Paul Davis

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

#include <iostream>

#include <gtkmm2ext/doi.h>

#include "ardour_dialog.h"
#include "keyboard.h"
#include "ardour_ui.h"


ArdourDialog::ArdourDialog (string title, bool modal, bool use_seperator)
	: Dialog (title, modal, use_seperator)
{
	session = 0;
}

ArdourDialog::~ArdourDialog ()
{
}

bool
ArdourDialog::on_enter_notify_event (GdkEventCrossing *ev)
{
	Keyboard::the_keyboard().enter_window (ev, this);
	return false;
}

bool
ArdourDialog::on_leave_notify_event (GdkEventCrossing *ev)
{
	Keyboard::the_keyboard().leave_window (ev, this);
	return false;
}

void
ArdourDialog::on_unmap ()
{
	Dialog::on_unmap ();
}
