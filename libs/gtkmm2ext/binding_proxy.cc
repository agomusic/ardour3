/*
    Copyright (C) 2006 Paul Davis
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
#include <climits>
#include <iostream>

#include <pbd/controllable.h>

#include <gtkmm2ext/binding_proxy.h>

#include "i18n.h"

using namespace Gtkmm2ext;
using namespace std;
using namespace PBD;

BindingProxy::BindingProxy (Controllable& c)
	: prompter (Gtk::WIN_POS_MOUSE, 30000, false),
	  controllable (c),
	  bind_button (2),
	  bind_statemask (Gdk::CONTROL_MASK)

{			  
	prompter.signal_unmap_event().connect (mem_fun (*this, &BindingProxy::prompter_hiding));
}

void
BindingProxy::set_bind_button_state (guint button, guint statemask)
{
	bind_button = button;
	bind_statemask = statemask;
}

void
BindingProxy::get_bind_button_state (guint &button, guint &statemask)
{
	button = bind_button;
	statemask = bind_statemask;
}

bool
BindingProxy::button_press_handler (GdkEventButton *ev)
{
	if ((ev->state & bind_statemask) && ev->button == bind_button) { 
		if (Controllable::StartLearning (&controllable)) {
			string prompt = _("operate controller now");
			prompter.set_text (prompt);
			prompter.touch (); // shows popup
			learning_connection = controllable.LearningFinished.connect (mem_fun (*this, &BindingProxy::learning_finished));
		}
		return true;
	}
	
	return false;
}

void
BindingProxy::learning_finished ()
{
	learning_connection.disconnect ();
	prompter.touch (); // hides popup
}


bool
BindingProxy::prompter_hiding (GdkEventAny *ev)
{
	learning_connection.disconnect ();
	Controllable::StopLearning (&controllable);
	return false;
}

