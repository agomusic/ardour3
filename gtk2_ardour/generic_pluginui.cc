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

*/

#include <climits>
#include <cerrno>
#include <cmath>
#include <string>

#include <pbd/stl_delete.h>
#include <pbd/xml++.h>
#include <pbd/failed_constructor.h>

#include <gtkmm2ext/click_box.h>
#include <gtkmm2ext/fastmeter.h>
#include <gtkmm2ext/barcontroller.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/doi.h>
#include <gtkmm2ext/slider_controller.h>

#include <midi++/manager.h>

#include <ardour/plugin.h>
#include <ardour/plugin_insert.h>
#include <ardour/ladspa_plugin.h>
#ifdef HAVE_LV2
#include <ardour/lv2_plugin.h>
#endif

#include <lrdf.h>

#include "ardour_ui.h"
#include "prompter.h"
#include "plugin_ui.h"
#include "utils.h"
#include "gui_thread.h"
#include "automation_controller.h"

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtkmm2ext;
using namespace Gtk;
using namespace sigc;

GenericPluginUI::GenericPluginUI (boost::shared_ptr<PluginInsert> pi, bool scrollable)
	: PlugUIBase (pi),
	  button_table (initial_button_rows, initial_button_cols),
	  output_table (initial_output_rows, initial_output_cols),
	  hAdjustment(0.0, 0.0, 0.0),
	  vAdjustment(0.0, 0.0, 0.0),
	  scroller_view(hAdjustment, vAdjustment),
	  automation_menu (0),
	  is_scrollable(scrollable)
{
	set_name ("PluginEditor");
	set_border_width (10);
	set_homogeneous (false);

	settings_box.set_homogeneous (false);

	HBox* constraint_hbox = manage (new HBox);
	HBox* smaller_hbox = manage (new HBox);
	Label* combo_label = manage (new Label (_("<span size=\"large\">Presets</span>")));
	combo_label->set_use_markup (true);

	Label* latency_label = manage (new Label (_("<span size=\"large\">Latency</span>")));
	latency_label->set_use_markup (true);

	smaller_hbox->pack_start (*latency_label, false, false, 10);
	smaller_hbox->pack_start (latency_gui, false, false, 10);
	smaller_hbox->pack_start (preset_combo, false, false);
	smaller_hbox->pack_start (save_button, false, false);

	constraint_hbox->set_spacing (5);
	constraint_hbox->pack_start (*smaller_hbox, true, false);
	constraint_hbox->pack_end (bypass_button, false, false);

	settings_box.pack_end (*constraint_hbox, false, false);

	pack_start (settings_box, false, false);

	if ( is_scrollable ) {
		scroller.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		scroller.set_name ("PluginEditor");
		scroller_view.set_name("PluginEditor");
		scroller_view.add (hpacker);
		scroller.add (scroller_view);
		
		pack_start (scroller, true, true);

	}
	else {
		pack_start (hpacker, false, false);
	}

	pi->ActiveChanged.connect (bind(mem_fun(*this, &GenericPluginUI::processor_active_changed),
				boost::weak_ptr<Processor>(pi)));
	bypass_button.set_active (!pi->active());
	
	build ();
}

GenericPluginUI::~GenericPluginUI ()
{
	if (output_controls.size() > 0) {
		screen_update_connection.disconnect();
	}
}

void
GenericPluginUI::build ()

{
	guint32 i = 0;
	guint32 x = 0;
	Frame* frame;
	Frame* bt_frame;
	VBox* box;
	int output_row, output_col;
	int button_row, button_col;
	int output_rows, output_cols;
	int button_rows, button_cols;

	prefheight = 30;
	hpacker.set_spacing (10);

	output_rows = initial_output_rows;
	output_cols = initial_output_cols;
	button_rows = initial_button_rows;
	button_cols = initial_button_cols;
	output_row = 0;
	button_row = 0;
	output_col = 0;
	button_col = 0;

	button_table.set_homogeneous (false);
	button_table.set_row_spacings (2);
	button_table.set_col_spacings (2);
	output_table.set_homogeneous (true);
	output_table.set_row_spacings (2);
	output_table.set_col_spacings (2);
	button_table.set_border_width (5);
	output_table.set_border_width (5);

	hpacker.set_border_width (10);

	bt_frame = manage (new Frame);
	bt_frame->set_name ("BaseFrame");
	bt_frame->add (button_table);
	hpacker.pack_start(*bt_frame, true, true);

	box = manage (new VBox);
	box->set_border_width (5);
	box->set_spacing (1);

	frame = manage (new Frame);
	frame->set_name ("BaseFrame");
	frame->set_label (_("Controls"));
	frame->add (*box);
	hpacker.pack_start(*frame, true, true);

	/* find all ports. build control elements for all appropriate control ports */

	for (i = 0; i < plugin->parameter_count(); ++i) {

		if (plugin->parameter_is_control (i)) {
			
			/* Don't show latency control ports */

			if (plugin->describe_parameter (Parameter(PluginAutomation, i)) == X_("latency")) {
				continue;
			}

			ControlUI* cui;
	
			/* if we are scrollable, just use one long column */

			if (!is_scrollable) {
				if (x++ > 7){
					frame = manage (new Frame);
					frame->set_name ("BaseFrame");
					box = manage (new VBox);
					
					box->set_border_width (5);
					box->set_spacing (1);

					frame->add (*box);
					hpacker.pack_start(*frame,true,true);

					x = 1;
				}
			}

			if ((cui = build_control_ui (i, insert->control(Parameter(PluginAutomation, i)))) == 0) {
				error << string_compose(_("Plugin Editor: could not build control element for port %1"), i) << endmsg;
				continue;
			}
				
			if (cui->control || cui->clickbox || cui->combo) {

				box->pack_start (*cui, false, false);

			} else if (cui->button) {

				if (button_row == button_rows) {
					button_row = 0;
					if (++button_col == button_cols) {
						button_cols += 2;
						button_table.resize (button_rows, button_cols);
					}
				}

				button_table.attach (*cui, button_col, button_col + 1, button_row, button_row+1, 
						     FILL|EXPAND, FILL);
				button_row++;

			} else if (cui->display) {

				output_table.attach (*cui, output_col, output_col + 1, output_row, output_row+1, 
						     FILL|EXPAND, FILL);
				
 				// TODO: The meters should be divided into multiple rows 
				
				if (++output_col == output_cols) {
					output_cols ++;
					output_table.resize (output_rows, output_cols);
				}
				
				/* old code, which divides meters into
				 * columns first, rows later. New code divides into one row
				 
				if (output_row == output_rows) {
					output_row = 0;
					if (++output_col == output_cols) {
						output_cols += 2;
						output_table.resize (output_rows, output_cols);
					}
				}
				
				output_table.attach (*cui, output_col, output_col + 1, output_row, output_row+1, 
						     FILL|EXPAND, FILL);
 
				output_row++;
				*/
			}
				
			/* HACK: ideally the preferred height would be queried from
			   the complete hpacker, but I can't seem to get that
			   information in time, so this is an estimation 
			*/

			prefheight += 30;

		} 
	}

	if (box->children().empty()) {
		hpacker.remove (*frame);
	}

	if (button_table.children().empty()) {
		hpacker.remove (*bt_frame);
	}

	if (!output_table.children().empty()) {
		frame = manage (new Frame);
		frame->set_name ("BaseFrame");
		frame->add (output_table);
		hpacker.pack_end (*frame, true, true);
	}

	output_update ();

	output_table.show_all ();
	button_table.show_all ();
}

GenericPluginUI::ControlUI::ControlUI ()
	: automate_button (X_("")) // force creation of a label 
{
	automate_button.set_name ("PluginAutomateButton");
	ARDOUR_UI::instance()->tooltips().set_tip (automate_button, _("Automation control"));

	/* XXX translators: use a string here that will be at least as long
	   as the longest automation label (see ::automation_state_changed()
	   below). be sure to include a descender.
	*/

	set_size_request_to_display_given_text (*automate_button.get_child(), _("Mgnual"), 5, 5);

	ignore_change = 0;
	display = 0;
	button = 0;
	clickbox = 0;
	meterinfo = 0;
}

GenericPluginUI::ControlUI::~ControlUI() 
{
	if (meterinfo) {
		delete meterinfo->meter;
		delete meterinfo;
	}
}

void
GenericPluginUI::automation_state_changed (ControlUI* cui)
{
	/* update button label */

	// don't lock to avoid deadlock because we're triggered by
	// AutomationControl::Changed() while the automation lock is taken
	switch (insert->get_parameter_automation_state (cui->parameter(), false)
			& (Off|Play|Touch|Write)) {
	case Off:
		cui->automate_button.set_label (_("Manual"));
		break;
	case Play:
		cui->automate_button.set_label (_("Play"));
		break;
	case Write:
		cui->automate_button.set_label (_("Write"));
		break;
	case Touch:
		cui->automate_button.set_label (_("Touch"));
		break;
	default:
		cui->automate_button.set_label (_("???"));
		break;
	}
}


static void integer_printer (char buf[32], Adjustment &adj, void *arg)
{
	snprintf (buf, 32, "%.0f", adj.get_value());
}

void
GenericPluginUI::print_parameter (char *buf, uint32_t len, uint32_t param)
{
	plugin->print_parameter (param, buf, len);
}

GenericPluginUI::ControlUI*
GenericPluginUI::build_control_ui (guint32 port_index, boost::shared_ptr<AutomationControl> mcontrol)
{
	ControlUI* control_ui = NULL;
	if (!mcontrol)
		return control_ui;

	Plugin::ParameterDescriptor desc;

	plugin->get_parameter_descriptor (port_index, desc);

	control_ui = manage (new ControlUI ());
	control_ui->combo = 0;
	control_ui->combo_map = 0;
	control_ui->control = mcontrol;
	control_ui->update_pending = false;
	control_ui->label.set_text (desc.label);
	control_ui->label.set_alignment (0.0, 0.5);
	control_ui->label.set_name ("PluginParameterLabel");

	control_ui->set_spacing (5);

	Gtk::Requisition req (control_ui->automate_button.size_request());

	if (plugin->parameter_is_input (port_index)) {

		boost::shared_ptr<LadspaPlugin> lp;
#ifdef HAVE_LV2
		boost::shared_ptr<LV2Plugin> lv2p;
#endif
		if ((lp = boost::dynamic_pointer_cast<LadspaPlugin>(plugin)) != 0) {
			
			// FIXME: not all plugins have a numeric unique ID
			uint32_t id = atol (lp->unique_id().c_str());
			lrdf_defaults* defaults = lrdf_get_scale_values(id, port_index);
			
			if (defaults && defaults->count > 0)	{
				
				control_ui->combo = new Gtk::ComboBoxText;
				//control_ui->combo->set_value_in_list(true, false);
				set_popdown_strings (*control_ui->combo, setup_scale_values(port_index, control_ui));
				control_ui->combo->signal_changed().connect (bind (mem_fun(*this, &GenericPluginUI::control_combo_changed), control_ui));
				mcontrol->Changed.connect (bind (mem_fun (*this, &GenericPluginUI::parameter_changed), control_ui));
				control_ui->pack_start(control_ui->label, true, true);
				control_ui->pack_start(*control_ui->combo, false, true);
				
				update_control_display(control_ui);
				
				lrdf_free_setting_values(defaults);
				return control_ui;
			}

#ifdef HAVE_LV2
		} else if ((lv2p = boost::dynamic_pointer_cast<LV2Plugin>(plugin)) != 0) {

			SLV2Port port = lv2p->slv2_port(port_index);
			SLV2ScalePoints points = slv2_port_get_scale_points(lv2p->slv2_plugin(), port);

			if (points) {
				control_ui->combo = new Gtk::ComboBoxText;
				//control_ui->combo->set_value_in_list(true, false);
				set_popdown_strings (*control_ui->combo, setup_scale_values(port_index, control_ui));
				control_ui->combo->signal_changed().connect (bind (mem_fun(*this, &GenericPluginUI::control_combo_changed), control_ui));
				mcontrol->Changed.connect (bind (mem_fun (*this, &GenericPluginUI::parameter_changed), control_ui));
				control_ui->pack_start(control_ui->label, true, true);
				control_ui->pack_start(*control_ui->combo, false, true);
				
				update_control_display(control_ui);
				
				slv2_scale_points_free(points);
				return control_ui;
			}
#endif
		}
			
		if (desc.toggled) {

			/* Build a button */
		
			control_ui->button = manage (new ToggleButton ());
			control_ui->button->set_name ("PluginEditorButton");
			control_ui->button->set_size_request (20, 20);

			control_ui->pack_start (control_ui->label, true, true);
			control_ui->pack_start (*control_ui->button, false, true);
			control_ui->pack_start (control_ui->automate_button, false, false);

			control_ui->button->signal_clicked().connect (bind (mem_fun(*this, &GenericPluginUI::control_port_toggled), control_ui));
		
			if(plugin->get_parameter (port_index) == 1){
				control_ui->button->set_active(true);
			}

			return control_ui;
		}

		/* create the controller */
	
		control_ui->controller = AutomationController::create(insert, mcontrol->list(), mcontrol);

		/* XXX this code is not right yet, because it doesn't handle
		   the absence of bounds in any sensible fashion.
		*/

#if 0
		control_ui->controller->adjustment()->set_lower (desc.lower);
		control_ui->controller->adjustment()->set_upper (desc.upper);

		control_ui->logarithmic = desc.logarithmic;
		if (control_ui->logarithmic) {
			if (control_ui->controller->adjustment()->get_lower() == 0.0) {
				control_ui->controller->adjustment()->set_lower (control_ui->controller->adjustment()->get_upper()/10000);
			}
			control_ui->controller->adjustment()->set_upper (log(control_ui->controller->adjustment()->get_upper()));
			control_ui->controller->adjustment()->set_lower (log(control_ui->controller->adjustment()->get_lower()));
		}
		
	
		float delta = desc.upper - desc.lower;

		control_ui->controller->adjustment()->set_page_size (delta/100.0);
		control_ui->controller->adjustment()->set_step_increment (desc.step);
		control_ui->controller->adjustment()->set_page_increment (desc.largestep);
#endif

		if (desc.integer_step) {
			control_ui->clickbox = new ClickBox (control_ui->controller->adjustment(), "PluginUIClickBox");
			Gtkmm2ext::set_size_request_to_display_given_text (*control_ui->clickbox, "g9999999", 2, 2);
			control_ui->clickbox->set_print_func (integer_printer, 0);
		} else {
			//sigc::slot<void,char*,uint32_t> pslot = sigc::bind (mem_fun(*this, &GenericPluginUI::print_parameter), (uint32_t) port_index);

			control_ui->controller->set_size_request (200, req.height);
			control_ui->controller->set_name (X_("PluginSlider"));
			control_ui->controller->set_style (BarController::LeftToRight);
			control_ui->controller->set_use_parent (true);

			control_ui->controller->StartGesture.connect (bind (mem_fun(*this, &GenericPluginUI::start_touch), control_ui));
			control_ui->controller->StopGesture.connect (bind (mem_fun(*this, &GenericPluginUI::stop_touch), control_ui));
			
		}

		if (control_ui->logarithmic) {
			control_ui->controller->adjustment()->set_value(log(plugin->get_parameter(port_index)));
		} else{
			control_ui->controller->adjustment()->set_value(plugin->get_parameter(port_index));
		}

		/* XXX memory leak: SliderController not destroyed by ControlUI
		   destructor, and manage() reports object hierarchy
		   ambiguity.
		*/

		control_ui->pack_start (control_ui->label, true, true);
		if (desc.integer_step) {
			control_ui->pack_start (*control_ui->clickbox, false, false);
		} else {
			control_ui->pack_start (*control_ui->controller, false, false);
		}

		control_ui->pack_start (control_ui->automate_button, false, false);
		control_ui->automate_button.signal_clicked().connect (bind (mem_fun(*this, &GenericPluginUI::astate_clicked), control_ui, (uint32_t) port_index));

		automation_state_changed (control_ui);

		mcontrol->Changed.connect (bind (mem_fun (*this, &GenericPluginUI::parameter_changed), control_ui));
		mcontrol->list()->automation_state_changed.connect 
			(bind (mem_fun(*this, &GenericPluginUI::automation_state_changed), control_ui));

	} else if (plugin->parameter_is_output (port_index)) {

		control_ui->display = manage (new EventBox);
		control_ui->display->set_name ("ParameterValueDisplay");

		control_ui->display_label = manage (new Label);

		control_ui->display_label->set_name ("ParameterValueDisplay");

		control_ui->display->add (*control_ui->display_label);
		Gtkmm2ext::set_size_request_to_display_given_text (*control_ui->display, "-99,99", 2, 2);

		control_ui->display->show_all ();

		/* set up a meter */
		/* TODO: only make a meter if the port is Hinted for it */

		MeterInfo * info = new MeterInfo(port_index);
 		control_ui->meterinfo = info;
		
		info->meter = new FastMeter (5, 5, FastMeter::Vertical);

		info->min_unbound = desc.min_unbound;
		info->max_unbound = desc.max_unbound;

		info->min = desc.lower;
		info->max = desc.upper;

		control_ui->vbox = manage (new VBox);
		control_ui->hbox = manage (new HBox);
		
		control_ui->label.set_angle(90);
		control_ui->hbox->pack_start (control_ui->label, false, false);
 		control_ui->hbox->pack_start (*info->meter, false, false);

 		control_ui->vbox->pack_start (*control_ui->hbox, false, false);
		
 		control_ui->vbox->pack_start (*control_ui->display, false, false);

		control_ui->pack_start (*control_ui->vbox);

		control_ui->meterinfo->meter->show_all();
		control_ui->meterinfo->packed = true;
		
		output_controls.push_back (control_ui);
	}
	
	mcontrol->Changed.connect (bind (mem_fun (*this, &GenericPluginUI::parameter_changed), control_ui));
	
	return control_ui;
}

void
GenericPluginUI::start_touch (GenericPluginUI::ControlUI* cui)
{
	cui->control->list()->start_touch ();
}

void
GenericPluginUI::stop_touch (GenericPluginUI::ControlUI* cui)
{
	cui->control->list()->stop_touch ();
}

void
GenericPluginUI::astate_clicked (ControlUI* cui, uint32_t port)
{
	using namespace Menu_Helpers;

	if (automation_menu == 0) {
		automation_menu = manage (new Menu);
		automation_menu->set_name ("ArdourContextMenu");
	} 

	MenuList& items (automation_menu->items());

	items.clear ();
	items.push_back (MenuElem (_("Manual"), 
				   bind (mem_fun(*this, &GenericPluginUI::set_automation_state), (AutoState) Off, cui)));
	items.push_back (MenuElem (_("Play"),
				   bind (mem_fun(*this, &GenericPluginUI::set_automation_state), (AutoState) Play, cui)));
	items.push_back (MenuElem (_("Write"),
				   bind (mem_fun(*this, &GenericPluginUI::set_automation_state), (AutoState) Write, cui)));
	items.push_back (MenuElem (_("Touch"),
				   bind (mem_fun(*this, &GenericPluginUI::set_automation_state), (AutoState) Touch, cui)));

	automation_menu->popup (1, gtk_get_current_event_time());
}

void
GenericPluginUI::set_automation_state (AutoState state, ControlUI* cui)
{
	insert->set_parameter_automation_state (cui->parameter(), state);
}

void
GenericPluginUI::parameter_changed (ControlUI* cui)
{
	if (!cui->update_pending) {
		cui->update_pending = true;
		Gtkmm2ext::UI::instance()->call_slot (bind (mem_fun(*this, &GenericPluginUI::update_control_display), cui));
	}
}

void
GenericPluginUI::update_control_display (ControlUI* cui)	
{
	/* XXX how do we handle logarithmic stuff here ? */
	
	cui->update_pending = false;

	float val = cui->control->get_value();

	cui->ignore_change++;
	if (cui->combo) {
	        std::map<string,float>::iterator it;
		for (it = cui->combo_map->begin(); it != cui->combo_map->end(); ++it) {
			if (it->second == val) {
				cui->combo->set_active_text(it->first);
				break;
			}
		}
	} else if (cui->button) {

		if (val > 0.5) {
			cui->button->set_active (true);
		} else {
			cui->button->set_active (false);
		}
	}

	cui->controller->display_effective_value();


	/*} else {
		if (cui->logarithmic) {
			val = log(val);
		}
		if (val != cui->adjustment->get_value()) {
			cui->adjustment->set_value (val);
		}
	}*/
	cui->ignore_change--;
}

void
GenericPluginUI::control_port_toggled (ControlUI* cui)
{
	if (!cui->ignore_change) {
		insert->set_parameter (cui->parameter(), cui->button->get_active());
	}
}

void
GenericPluginUI::control_combo_changed (ControlUI* cui)
{
	if (!cui->ignore_change) {
		string value = cui->combo->get_active_text();
		std::map<string,float> mapping = *cui->combo_map;
		insert->set_parameter (cui->parameter(), mapping[value]);
	}

}

void
GenericPluginUI::processor_active_changed (boost::weak_ptr<Processor> weak_processor)
{
	ENSURE_GUI_THREAD(bind (mem_fun(*this, &GenericPluginUI::processor_active_changed), weak_processor));
	
	boost::shared_ptr<Processor> processor = weak_processor.lock();

	bypass_button.set_active (!processor || !processor->active());
}

bool
GenericPluginUI::start_updating (GdkEventAny* ignored)
{
	if (output_controls.size() > 0 ) {
		screen_update_connection.disconnect();
		screen_update_connection = ARDOUR_UI::instance()->RapidScreenUpdate.connect 
			(mem_fun(*this, &GenericPluginUI::output_update));
	}
	return false;
}

bool
GenericPluginUI::stop_updating (GdkEventAny* ignored)
{
	if (output_controls.size() > 0 ) {
		screen_update_connection.disconnect();
	}
	return false;
}

void
GenericPluginUI::output_update ()
{
	for (vector<ControlUI*>::iterator i = output_controls.begin(); i != output_controls.end(); ++i) {
		float val = plugin->get_parameter ((*i)->parameter().id());
		char buf[32];
		snprintf (buf, sizeof(buf), "%.2f", val);
		(*i)->display_label->set_text (buf);

		/* autoscaling for the meter */
		if ((*i)->meterinfo && (*i)->meterinfo->packed) {
			
			if (val < (*i)->meterinfo->min) {
				if ((*i)->meterinfo->min_unbound)
					(*i)->meterinfo->min = val;
				else
					val = (*i)->meterinfo->min;
			}

			if (val > (*i)->meterinfo->max) {
				if ((*i)->meterinfo->max_unbound)
					(*i)->meterinfo->max = val;
				else
					val = (*i)->meterinfo->max;
			}
			
			if ((*i)->meterinfo->max > (*i)->meterinfo->min ) {
				float lval = (val - (*i)->meterinfo->min) / ((*i)->meterinfo->max - (*i)->meterinfo->min) ;
				(*i)->meterinfo->meter->set (lval );
			}
		}
	}
}

vector<string> 
GenericPluginUI::setup_scale_values(guint32 port_index, ControlUI* cui)
{
	vector<string> enums;
	boost::shared_ptr<LadspaPlugin> lp;
#ifdef HAVE_LV2
	boost::shared_ptr<LV2Plugin> lv2p;
#endif

	if ((lp = boost::dynamic_pointer_cast<LadspaPlugin>(plugin)) != 0) {
		// all LADPSA plugins have a numeric unique ID
		uint32_t id = atol (lp->unique_id().c_str());

		cui->combo_map = new std::map<string, float>;
		lrdf_defaults* defaults = lrdf_get_scale_values(id, port_index);
		if (defaults)	{
			for (uint32_t i = 0; i < defaults->count; ++i) {
				enums.push_back(defaults->items[i].label);
				pair<string, float> newpair;
				newpair.first = defaults->items[i].label;
				newpair.second = defaults->items[i].value;
				cui->combo_map->insert(newpair);
			}

			lrdf_free_setting_values(defaults);
		}

#ifdef HAVE_LV2
	} else if ((lv2p = boost::dynamic_pointer_cast<LV2Plugin>(plugin)) != 0) {

		SLV2Port port = lv2p->slv2_port(port_index);
		SLV2ScalePoints points = slv2_port_get_scale_points(lv2p->slv2_plugin(), port);
		cui->combo_map = new std::map<string, float>;
	
		for (unsigned i=0; i < slv2_scale_points_size(points); ++i) {
			SLV2ScalePoint p = slv2_scale_points_get_at(points, i);
			SLV2Value label = slv2_scale_point_get_label(p);
			SLV2Value value = slv2_scale_point_get_value(p);
			if (label && (slv2_value_is_float(value) || slv2_value_is_int(value))) {
				enums.push_back(slv2_value_as_string(label));
				pair<string, float> newpair;
				newpair.first = slv2_value_as_string(label);
				newpair.second = slv2_value_as_float(value);
				cui->combo_map->insert(newpair);
			}
		}

		slv2_scale_points_free(points);
#endif
	}
	

	return enums;
}
