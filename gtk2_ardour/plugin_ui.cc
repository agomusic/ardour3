/*
    Copyright (C) 2000 Paul Davis 

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

#include <climits>
#include <cerrno>
#include <cmath>
#include <string>

#include <pbd/stl_delete.h>
#include <pbd/xml++.h>
#include <pbd/failed_constructor.h>

#include <gtkmm/widget.h>
#include <gtkmm2ext/click_box.h>
#include <gtkmm2ext/fastmeter.h>
#include <gtkmm2ext/barcontroller.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/doi.h>
#include <gtkmm2ext/slider_controller.h>

#include <midi++/manager.h>

#include <ardour/plugin.h>
#include <ardour/insert.h>
#include <ardour/ladspa_plugin.h>
#ifdef VST_SUPPORT
#include <ardour/vst_plugin.h>
#endif

#include <lrdf.h>

#include "ardour_ui.h"
#include "prompter.h"
#include "plugin_ui.h"
#include "utils.h"
#include "gui_thread.h"
#include "public_editor.h"

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtkmm2ext;
using namespace Gtk;
using namespace sigc;

PluginUIWindow::PluginUIWindow (boost::shared_ptr<PluginInsert> insert, bool scrollable)
	: ArdourDialog ("plugin ui")
{
	if (insert->plugin()->has_editor()) {

#ifdef VST_SUPPORT

		boost::shared_ptr<VSTPlugin> vp;

		if ((vp = boost::dynamic_pointer_cast<VSTPlugin> (insert->plugin())) != 0) {
			
			
			VSTPluginUI* vpu = new VSTPluginUI (insert, vp);
			
			_pluginui = vpu;
			get_vbox()->add (*vpu);
			vpu->package (*this);
			
		} else {
#endif
			error << _("unknown type of editor-supplying plugin (note: no VST support in this version of ardour)")
			      << endmsg;
			throw failed_constructor ();
#ifdef VST_SUPPORT
		}
#endif

	} else {

		LadspaPluginUI*  pu  = new LadspaPluginUI (insert, scrollable);
		
		_pluginui = pu;
		get_vbox()->add (*pu);
		
		set_wmclass (X_("ardour_plugin_editor"), "Ardour");

		signal_map_event().connect (mem_fun (*pu, &LadspaPluginUI::start_updating));
		signal_unmap_event().connect (mem_fun (*pu, &LadspaPluginUI::stop_updating));
	}

	set_position (Gtk::WIN_POS_MOUSE);
	set_name ("PluginEditor");
	add_events (Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK|Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);

	signal_delete_event().connect (bind (sigc::ptr_fun (just_hide_it), reinterpret_cast<Window*> (this)));
	insert->GoingAway.connect (mem_fun(*this, &PluginUIWindow::plugin_going_away));

	if (scrollable) {
		gint h = _pluginui->get_preferred_height ();
		if (h > 600) h = 600;
		set_default_size (450, h); 
	}

}

PluginUIWindow::~PluginUIWindow ()
{
}

bool
PluginUIWindow::on_key_press_event (GdkEventKey* event)
{
	if (!key_press_focus_accelerator_handler (*this, event)) {
		return PublicEditor::instance().on_key_press_event(event);
	} else {
		return true;
	}
}

bool
PluginUIWindow::on_key_release_event (GdkEventKey* event)
{
	return true;
}

void
PluginUIWindow::plugin_going_away ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &PluginUIWindow::plugin_going_away));
	
	_pluginui->stop_updating(0);
	delete_when_idle (this);
}

PlugUIBase::PlugUIBase (boost::shared_ptr<PluginInsert> pi)
	: insert (pi),
	  plugin (insert->plugin()),
	  save_button(_("Add")),
	  bypass_button (_("Bypass"))
{
        //combo.set_use_arrows_always(true);
	set_popdown_strings (combo, plugin->get_presets());
	combo.set_size_request (100, -1);
	combo.set_active_text ("");
	combo.signal_changed().connect(mem_fun(*this, &PlugUIBase::setting_selected));

	save_button.set_name ("PluginSaveButton");
	save_button.signal_clicked().connect(mem_fun(*this, &PlugUIBase::save_plugin_setting));

	bypass_button.set_name ("PluginBypassButton");
	bypass_button.signal_toggled().connect (mem_fun(*this, &PlugUIBase::bypass_toggled));
}

void
PlugUIBase::setting_selected()
{
	if (combo.get_active_text().length() > 0) {
		if (!plugin->load_preset(combo.get_active_text())) {
			warning << string_compose(_("Plugin preset %1 not found"), combo.get_active_text()) << endmsg;
		}
	}

}

void
PlugUIBase::save_plugin_setting ()
{
	ArdourPrompter prompter (true);
	prompter.set_prompt(_("Name of New Preset:"));
	prompter.add_button (Gtk::Stock::ADD, Gtk::RESPONSE_ACCEPT);
	prompter.set_response_sensitive (Gtk::RESPONSE_ACCEPT, false);

	prompter.show_all();

	switch (prompter.run ()) {
	case Gtk::RESPONSE_ACCEPT:

		string name;

		prompter.get_result(name);

		if (name.length()) {
			if(plugin->save_preset(name)){
				set_popdown_strings (combo, plugin->get_presets());
				combo.set_active_text (name);
			}
		}
		break;
	}
}

void
PlugUIBase::bypass_toggled ()
{
	bool x;

	if ((x = bypass_button.get_active()) == insert->active()) {
		insert->set_active (!x, this);
	}
}

