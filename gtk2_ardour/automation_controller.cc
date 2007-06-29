/*
    Copyright (C) 2007 Paul Davis 
	Author: Dave Robillard

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

#include <pbd/error.h>
#include "automation_controller.h"
#include "ardour/automation_event.h"
#include "ardour/automation_control.h"
#include "ardour_ui.h"
#include "utils.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtk;


AutomationController::AutomationController(boost::shared_ptr<AutomationControl> ac, Adjustment* adj)
	: BarController(*adj, *ac)
	, _ignore_change(false)
	, _controllable(ac)
	, _adjustment(adj)
{
	set_name (X_("PluginSlider")); // FIXME: get yer own name!
	set_style (BarController::LeftToRight);
	set_use_parent (true);
	
	label_callback = sigc::mem_fun(this, &AutomationController::update_label);
	
	StartGesture.connect (mem_fun(*this, &AutomationController::start_touch));
	StopGesture.connect (mem_fun(*this, &AutomationController::end_touch));
	
	_adjustment->signal_value_changed().connect (
			mem_fun(*this, &AutomationController::value_adjusted));
		
	_screen_update_connection = ARDOUR_UI::RapidScreenUpdate.connect (
			mem_fun (*this, &AutomationController::display_effective_value));
}

AutomationController::~AutomationController()
{
}

boost::shared_ptr<AutomationController>
AutomationController::create(Session& s, boost::shared_ptr<AutomationList> al, boost::shared_ptr<AutomationControl> ac)
{
	Gtk::Adjustment* adjustment = manage(new Gtk::Adjustment(al->default_value(), al->get_min_y(), al->get_max_y()));
	if (!ac) {
		PBD::warning << "Creating AutomationController for " << al->param_id().to_string() << endmsg;
		ac = boost::shared_ptr<AutomationControl>(new AutomationControl(s, al));
	}
	return boost::shared_ptr<AutomationController>(new AutomationController(ac, adjustment));
}

void
AutomationController::update_label(char* label, int label_len)
{
	//cerr << "Controller label: " << label << endl;
}

void
AutomationController::display_effective_value()
{
	if ( ! _controllable->list()->automation_playback())
		return;

	float value = _controllable->get_value();
	
	if (_adjustment->get_value() != value) {
		_ignore_change = true; 
		_adjustment->set_value (value);
		_ignore_change = false;
	}
}

void
AutomationController::value_adjusted()
{
	if (!_ignore_change) {
		_controllable->set_value(_adjustment->get_value());
	}
}

void
AutomationController::start_touch()
{
	_controllable->list()->start_touch();
}

void
AutomationController::end_touch()
{
	_controllable->list()->stop_touch();
}