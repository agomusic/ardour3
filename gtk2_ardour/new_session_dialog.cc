/*
    Copyright (C) 2005 Paul Davis 

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

#include "i18n.h"
#include "new_session_dialog.h"

#include <ardour/recent_sessions.h>
#include <ardour/session.h>

#include <gtkmm/entry.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/stock.h>

#include "opts.h"

NewSessionDialog::NewSessionDialog()
	: ArdourDialog ("New Session Dialog")
{
        session_name_label = Gtk::manage(new class Gtk::Label(_("New Session Name :")));
	m_name = Gtk::manage(new class Gtk::Entry());
	m_name->set_text(GTK_ARDOUR::session_name);

	session_location_label = Gtk::manage(new class Gtk::Label(_("Create Session Directory In :")));
	m_folder = Gtk::manage(new class Gtk::FileChooserButton(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER));
	session_template_label = Gtk::manage(new class Gtk::Label(_("Use Session Template :")));
	m_template = Gtk::manage(new class Gtk::FileChooserButton());
	chan_count_label = Gtk::manage(new class Gtk::Label(_("Channel Count")));
	m_create_control_bus = Gtk::manage(new class Gtk::CheckButton(_("Create Monitor Bus")));
	
	Gtk::Adjustment *m_control_bus_channel_count_adj = Gtk::manage(new class Gtk::Adjustment(2, 0, 100, 1, 10, 10));
	m_control_bus_channel_count = Gtk::manage(new class Gtk::SpinButton(*m_control_bus_channel_count_adj, 1, 0));
	
	Gtk::Adjustment *m_master_bus_channel_count_adj = Gtk::manage(new class Gtk::Adjustment(2, 0, 100, 1, 10, 10));
	m_master_bus_channel_count = Gtk::manage(new class Gtk::SpinButton(*m_master_bus_channel_count_adj, 1, 0));
	m_create_master_bus = Gtk::manage(new class Gtk::CheckButton(_("Create Master Bus")));
	advanced_table = Gtk::manage(new class Gtk::Table(2, 2, true));
	m_connect_inputs = Gtk::manage(new class Gtk::CheckButton(_("Automatically Connect Inputs")));
	m_limit_input_ports = Gtk::manage(new class Gtk::CheckButton(_("Port Limit")));
	
	Gtk::Adjustment *m_input_limit_count_adj = Gtk::manage(new class Gtk::Adjustment(1, 0, 100, 1, 10, 10));
	m_input_limit_count = Gtk::manage(new class Gtk::SpinButton(*m_input_limit_count_adj, 1, 0));
	input_port_limit_hbox = Gtk::manage(new class Gtk::HBox(false, 0));
	input_port_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	input_table = Gtk::manage(new class Gtk::Table(2, 2, false));
	input_port_alignment = Gtk::manage(new class Gtk::Alignment(0.5, 0.5, 1, 1));
	input_label = Gtk::manage(new class Gtk::Label(_("<b>Track/Bus Inputs</b>")));
	input_frame = Gtk::manage(new class Gtk::Frame());
	m_connect_outputs = Gtk::manage(new class Gtk::CheckButton(_("Automatically Connect Outputs")));
	m_limit_output_ports = Gtk::manage(new class Gtk::CheckButton(_("Port Limit")));
	
	Gtk::Adjustment *m_output_limit_count_adj = Gtk::manage(new class Gtk::Adjustment(1, 0, 100, 1, 10, 10));
	m_output_limit_count = Gtk::manage(new class Gtk::SpinButton(*m_output_limit_count_adj, 1, 0));
	output_port_limit_hbox = Gtk::manage(new class Gtk::HBox(false, 0));
	output_port_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	
	Gtk::RadioButton::Group _RadioBGroup_m_connect_outputs_to_master;
	m_connect_outputs_to_master = Gtk::manage(new class Gtk::RadioButton(_RadioBGroup_m_connect_outputs_to_master, _("Connect to Master Bus")));
	m_connect_outputs_to_physical = Gtk::manage(new class Gtk::RadioButton(_RadioBGroup_m_connect_outputs_to_master, _("Connect to Physical Outputs")));
	output_conn_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	output_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	output_port_alignment = Gtk::manage(new class Gtk::Alignment(0.5, 0.5, 1, 1));
	output_label = Gtk::manage(new class Gtk::Label(_("<b>Track/Bus Outputs</b>")));
	output_frame = Gtk::manage(new class Gtk::Frame());
	advanced_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	advanced_label = Gtk::manage(new class Gtk::Label(_("Advanced Options")));
	advanced_expander = Gtk::manage(new class Gtk::Expander());
	new_session_table = Gtk::manage(new class Gtk::Table(2, 2, false));
	m_open_filechooser = Gtk::manage(new class Gtk::FileChooserButton());
	open_session_hbox = Gtk::manage(new class Gtk::HBox(false, 0));
	m_treeview = Gtk::manage(new class Gtk::TreeView());
	recent_scrolledwindow = Gtk::manage(new class Gtk::ScrolledWindow());
	recent_alignment = Gtk::manage(new class Gtk::Alignment(0.5, 0.5, 1, 1));
	recent_sesion_label = Gtk::manage(new class Gtk::Label(_("Open Recent Session")));
	recent_frame = Gtk::manage(new class Gtk::Frame());
	open_session_vbox = Gtk::manage(new class Gtk::VBox(false, 0));
	m_notebook = Gtk::manage(new class Gtk::Notebook());
	session_name_label->set_alignment(0, 0.5);
	session_name_label->set_padding(0,0);
	session_name_label->set_line_wrap(false);
	session_name_label->set_selectable(false);
	m_name->set_editable(true);
	m_name->set_max_length(0);
	m_name->set_has_frame(true);
	m_name->set_activates_default(true);
	session_location_label->set_alignment(0,0.5);
	session_location_label->set_padding(0,0);
	session_location_label->set_line_wrap(false);
	session_location_label->set_selectable(false);
	session_template_label->set_alignment(0,0.5);
	session_template_label->set_padding(0,0);
	session_template_label->set_line_wrap(false);
	session_template_label->set_selectable(false);
	m_create_control_bus->set_flags(Gtk::CAN_FOCUS);
	m_create_control_bus->set_relief(Gtk::RELIEF_NORMAL);
	m_create_control_bus->set_mode(true);
	m_create_control_bus->set_active(false);
	m_create_control_bus->set_border_width(0);
	m_control_bus_channel_count->set_flags(Gtk::CAN_FOCUS);
	m_control_bus_channel_count->set_update_policy(Gtk::UPDATE_ALWAYS);
	m_control_bus_channel_count->set_numeric(true);
	m_control_bus_channel_count->set_digits(0);
	m_control_bus_channel_count->set_wrap(false);
	m_control_bus_channel_count->set_sensitive(false);
	m_master_bus_channel_count->set_flags(Gtk::CAN_FOCUS);
	m_master_bus_channel_count->set_update_policy(Gtk::UPDATE_ALWAYS);
	m_master_bus_channel_count->set_numeric(true);
	m_master_bus_channel_count->set_digits(0);
	m_master_bus_channel_count->set_wrap(false);
	open_session_file_label = Gtk::manage(new class Gtk::Label(_("Open Session File :")));
	open_session_file_label->set_alignment(0, 0.5);
	m_create_master_bus->set_flags(Gtk::CAN_FOCUS);
	m_create_master_bus->set_relief(Gtk::RELIEF_NORMAL);
	m_create_master_bus->set_mode(true);
	m_create_master_bus->set_active(true);
	m_create_master_bus->set_border_width(0);
	advanced_table->set_row_spacings(0);
	advanced_table->set_col_spacings(0);
	advanced_table->attach(*chan_count_label, 1, 2, 0, 1, Gtk::AttachOptions(), Gtk::AttachOptions(), 0, 0);
	advanced_table->attach(*m_create_control_bus, 0, 1, 2, 3, Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	advanced_table->attach(*m_control_bus_channel_count, 1, 2, 2, 3, Gtk::AttachOptions(), Gtk::AttachOptions(), 0, 0);
	advanced_table->attach(*m_master_bus_channel_count, 1, 2, 1, 2, Gtk::AttachOptions(), Gtk::AttachOptions(), 0, 0);
	advanced_table->attach(*m_create_master_bus, 0, 1, 1, 2, Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	m_connect_inputs->set_flags(Gtk::CAN_FOCUS);
	m_connect_inputs->set_relief(Gtk::RELIEF_NORMAL);
	m_connect_inputs->set_mode(true);
	m_connect_inputs->set_active(false);
	m_connect_inputs->set_border_width(0);
	m_limit_input_ports->set_flags(Gtk::CAN_FOCUS);
	m_limit_input_ports->set_relief(Gtk::RELIEF_NORMAL);
	m_limit_input_ports->set_mode(true);
	m_limit_input_ports->set_sensitive(false);
	m_limit_input_ports->set_border_width(0);
	m_input_limit_count->set_flags(Gtk::CAN_FOCUS);
	m_input_limit_count->set_update_policy(Gtk::UPDATE_ALWAYS);
	m_input_limit_count->set_numeric(true);
	m_input_limit_count->set_digits(0);
	m_input_limit_count->set_wrap(false);
	m_input_limit_count->set_sensitive(false);
	input_port_limit_hbox->pack_start(*m_limit_input_ports, Gtk::PACK_SHRINK, 6);
	input_port_limit_hbox->pack_start(*m_input_limit_count, Gtk::PACK_EXPAND_PADDING, 0);
	input_port_vbox->pack_start(*m_connect_inputs, Gtk::PACK_SHRINK, 0);
	input_port_vbox->pack_start(*input_port_limit_hbox, Gtk::PACK_EXPAND_PADDING, 0);
	input_table->set_row_spacings(0);
	input_table->set_col_spacings(0);
	input_table->attach(*input_port_vbox, 0, 1, 0, 1, Gtk::EXPAND|Gtk::FILL, Gtk::EXPAND|Gtk::FILL, 6, 6);
	input_port_alignment->add(*input_table);
	input_label->set_alignment(0, 0.5);
	input_label->set_padding(0,0);
	input_label->set_line_wrap(false);
	input_label->set_selectable(false);
	input_label->set_use_markup(true);
	input_frame->set_shadow_type(Gtk::SHADOW_NONE);
	input_frame->set_label_align(0,0.5);
	input_frame->add(*input_port_alignment);
	input_frame->set_label_widget(*input_label);
	m_connect_outputs->set_flags(Gtk::CAN_FOCUS);
	m_connect_outputs->set_relief(Gtk::RELIEF_NORMAL);
	m_connect_outputs->set_mode(true);
	m_connect_outputs->set_active(false);
	m_connect_outputs->set_border_width(0);
	m_limit_output_ports->set_flags(Gtk::CAN_FOCUS);
	m_limit_output_ports->set_relief(Gtk::RELIEF_NORMAL);
	m_limit_output_ports->set_mode(true);
	m_limit_output_ports->set_sensitive(false);
	m_limit_output_ports->set_border_width(0);
	m_output_limit_count->set_flags(Gtk::CAN_FOCUS);
	m_output_limit_count->set_update_policy(Gtk::UPDATE_ALWAYS);
	m_output_limit_count->set_numeric(false);
	m_output_limit_count->set_digits(0);
	m_output_limit_count->set_wrap(false);
	m_output_limit_count->set_sensitive(false);
	output_port_limit_hbox->pack_start(*m_limit_output_ports, Gtk::PACK_SHRINK, 6);
	output_port_limit_hbox->pack_start(*m_output_limit_count, Gtk::PACK_EXPAND_PADDING, 0);
	output_port_vbox->pack_start(*m_connect_outputs, Gtk::PACK_SHRINK, 0);
	output_port_vbox->pack_start(*output_port_limit_hbox, Gtk::PACK_EXPAND_PADDING, 0);
	m_connect_outputs_to_master->set_flags(Gtk::CAN_FOCUS);
	m_connect_outputs_to_master->set_relief(Gtk::RELIEF_NORMAL);
	m_connect_outputs_to_master->set_mode(true);
	m_connect_outputs_to_master->set_active(false);
	m_connect_outputs_to_master->set_border_width(0);
	m_connect_outputs_to_physical->set_flags(Gtk::CAN_FOCUS);
	m_connect_outputs_to_physical->set_relief(Gtk::RELIEF_NORMAL);
	m_connect_outputs_to_physical->set_mode(true);
	m_connect_outputs_to_physical->set_active(false);
	m_connect_outputs_to_physical->set_border_width(0);
	output_conn_vbox->pack_start(*m_connect_outputs_to_master, Gtk::PACK_SHRINK, 0);
	output_conn_vbox->pack_start(*m_connect_outputs_to_physical, Gtk::PACK_SHRINK, 0);
	output_vbox->set_border_width(6);
	output_vbox->pack_start(*output_port_vbox);
	output_vbox->pack_start(*output_conn_vbox);
	output_port_alignment->add(*output_vbox);
	output_label->set_alignment(0, 0.5);
	output_label->set_padding(0,0);
	output_label->set_line_wrap(false);
	output_label->set_selectable(false);
	output_label->set_use_markup(true);
	output_frame->set_shadow_type(Gtk::SHADOW_NONE);
	output_frame->set_label_align(0,0.5);
	output_frame->add(*output_port_alignment);
	output_frame->set_label_widget(*output_label);
	advanced_vbox->pack_start(*advanced_table, Gtk::PACK_SHRINK, 0);
	advanced_vbox->pack_start(*input_frame, Gtk::PACK_SHRINK, 12);
	advanced_vbox->pack_start(*output_frame, Gtk::PACK_SHRINK, 0);
	advanced_label->set_padding(0,0);
	advanced_label->set_line_wrap(false);
	advanced_label->set_selectable(false);
	advanced_label->set_alignment(0, 0.5);
	advanced_expander->set_flags(Gtk::CAN_FOCUS);
	advanced_expander->set_border_width(0);
	advanced_expander->set_expanded(false);
	advanced_expander->set_spacing(0);
	advanced_expander->add(*advanced_vbox);
	advanced_expander->set_label_widget(*advanced_label);
	new_session_table->set_border_width(12);
	new_session_table->set_row_spacings(0);
	new_session_table->set_col_spacings(0);
	new_session_table->attach(*session_name_label, 0, 1, 0, 1, Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*m_name, 1, 2, 0, 1, Gtk::EXPAND|Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*session_location_label, 0, 1, 1, 2, Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*m_folder, 1, 2, 1, 2, Gtk::EXPAND|Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*session_template_label, 0, 1, 2, 3, Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*m_template, 1, 2, 2, 3, Gtk::EXPAND|Gtk::FILL, Gtk::FILL, 0, 0);
	new_session_table->attach(*advanced_expander, 0, 2, 3, 4, Gtk::FILL, Gtk::EXPAND|Gtk::FILL, 0, 12);
	chan_count_label->set_padding(0,0);
	chan_count_label->set_line_wrap(false);
	chan_count_label->set_selectable(false);
	open_session_hbox->pack_start(*open_session_file_label, true, true, 12);
	open_session_hbox->pack_start(*m_open_filechooser, true, true, 12);
	m_treeview->set_flags(Gtk::CAN_FOCUS);
	m_treeview->set_headers_visible(true);
	m_treeview->set_rules_hint(false);
	m_treeview->set_reorderable(false);
	m_treeview->set_enable_search(true);
	m_treeview->set_fixed_height_mode(false);
	m_treeview->set_hover_selection(false);
	m_treeview->set_hover_expand(true);
	m_treeview->set_size_request(-1, 150);
	recent_scrolledwindow->set_flags(Gtk::CAN_FOCUS);
	recent_scrolledwindow->set_border_width(6);
	recent_scrolledwindow->set_shadow_type(Gtk::SHADOW_IN);
	recent_scrolledwindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	recent_scrolledwindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
	recent_scrolledwindow->add(*m_treeview);
	recent_alignment->add(*recent_scrolledwindow);
	recent_sesion_label->set_padding(0,0);
	recent_sesion_label->set_line_wrap(false);
	recent_sesion_label->set_selectable(false);
	recent_frame->set_border_width(12);
	recent_frame->set_shadow_type(Gtk::SHADOW_IN);
	recent_frame->add(*recent_alignment);
	recent_frame->set_label_widget(*recent_sesion_label);
	open_session_vbox->pack_start(*open_session_hbox, Gtk::PACK_SHRINK, 12);
	open_session_vbox->pack_start(*recent_frame, Gtk::PACK_EXPAND_WIDGET, 0);
	m_notebook->set_flags(Gtk::CAN_FOCUS);
	m_notebook->set_scrollable(true);
	m_notebook->append_page(*new_session_table, _("New Session"));
	m_notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	m_notebook->append_page(*open_session_vbox, _("Open Session"));
	m_notebook->pages().back().set_tab_label_packing(false, true, Gtk::PACK_START);
	get_vbox()->set_homogeneous(false);
	get_vbox()->set_spacing(0);
	get_vbox()->pack_start(*m_notebook, Gtk::PACK_SHRINK, 0);
	set_title(_("ardour: session control"));
	//set_modal(false);
	//property_window_position().set_value(Gtk::WIN_POS_NONE);
	set_resizable(false);
	//property_destroy_with_parent().set_value(false);
	set_has_separator(false);
	// add_button(Gtk::Stock::HELP, Gtk::RESPONSE_HELP);
	add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CANCEL);
	add_button(Gtk::Stock::CLEAR, Gtk::RESPONSE_NONE);
	m_okbutton = add_button(Gtk::Stock::NEW, Gtk::RESPONSE_OK);

	recent_model = Gtk::TreeStore::create (recent_columns);
	m_treeview->set_model (recent_model);
	m_treeview->append_column (_("Recent Sessions"), recent_columns.visible_name);
	m_treeview->set_headers_visible (false);
	m_treeview->get_selection()->set_mode (Gtk::SELECTION_SINGLE);

	std::string path = ARDOUR::get_user_ardour_path();
	if (path.empty()) {
	        path = ARDOUR::get_system_data_path();
	}
	if (!path.empty()) {
	        m_template->set_current_folder (path + X_("templates/"));
	}

	const std::string sys_templates_dir = ARDOUR::get_system_data_path() + X_("templates");
	if (Glib::file_test(sys_templates_dir, Glib::FILE_TEST_IS_DIR))
		m_template->add_shortcut_folder(sys_templates_dir);
	
	m_template->set_title(_("select template"));
	Gtk::FileFilter* session_filter = manage (new (Gtk::FileFilter));
	session_filter->add_pattern(X_("*.ardour"));
	session_filter->add_pattern(X_("*.ardour.bak"));
	m_open_filechooser->set_filter (*session_filter);
	m_open_filechooser->set_current_folder(getenv ("HOME"));
	m_open_filechooser->set_title(_("select session file"));

	Gtk::FileFilter* template_filter = manage (new (Gtk::FileFilter));
	template_filter->add_pattern(X_("*.ardour"));
	template_filter->add_pattern(X_("*.ardour.bak"));
	template_filter->add_pattern(X_("*.template"));
	m_template->set_filter (*template_filter);

	m_folder->set_current_folder(getenv ("HOME"));
	m_folder->set_title(_("select directory"));

	on_new_session_page = true;
	m_notebook->set_current_page(0);
	m_notebook->show();
	m_notebook->show_all_children();


	set_default_response (Gtk::RESPONSE_OK);
	if (!GTK_ARDOUR::session_name.length()) {
		set_response_sensitive (Gtk::RESPONSE_OK, false);
		set_response_sensitive (Gtk::RESPONSE_NONE, false);
	} else {
		set_response_sensitive (Gtk::RESPONSE_OK, true);
		set_response_sensitive (Gtk::RESPONSE_NONE, true);
	}

	///@ connect some signals

	m_connect_inputs->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::connect_inputs_clicked));
	m_connect_outputs->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::connect_outputs_clicked));
	m_limit_input_ports->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::limit_inputs_clicked));
	m_limit_output_ports->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::limit_outputs_clicked));
	m_create_master_bus->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::master_bus_button_clicked));
	m_create_control_bus->signal_clicked().connect (mem_fun (*this, &NewSessionDialog::monitor_bus_button_clicked));
	m_name->signal_key_release_event().connect(mem_fun (*this, &NewSessionDialog::entry_key_release));
	m_notebook->signal_switch_page().connect (mem_fun (*this, &NewSessionDialog::notebook_page_changed));
	m_treeview->get_selection()->signal_changed().connect (mem_fun (*this, &NewSessionDialog::treeview_selection_changed));
	m_treeview->signal_row_activated().connect (mem_fun (*this, &NewSessionDialog::recent_row_activated));
	m_open_filechooser->signal_selection_changed ().connect (mem_fun (*this, &NewSessionDialog::file_chosen));
	m_template->signal_selection_changed ().connect (mem_fun (*this, &NewSessionDialog::template_chosen));
	m_name->grab_focus();
}

void
NewSessionDialog::set_session_name(const Glib::ustring& name)
{
	m_name->set_text(name);
}

std::string
NewSessionDialog::session_name() const
{
        std::string str = Glib::filename_from_utf8(m_open_filechooser->get_filename());
	std::string::size_type position = str.find_last_of ('/');
	str = str.substr (position+1);
	position = str.find_last_of ('.');
	str = str.substr (0, position);

	/*
	  XXX what to do if it's a .bak file?
	  load_session doesn't allow it!

	if ((position = str.rfind(".bak")) != string::npos) {
	        str = str.substr (0, position);
	}	  
	*/

	if (m_notebook->get_current_page() == 0) {
	        return Glib::filename_from_utf8(m_name->get_text());
	} else {
		if (m_treeview->get_selection()->count_selected_rows() == 0) {
		        return Glib::filename_from_utf8(str);
		}
		Gtk::TreeModel::iterator i = m_treeview->get_selection()->get_selected();
		return (*i)[recent_columns.visible_name];
	}
}

std::string
NewSessionDialog::session_folder() const
{
        if (m_notebook->get_current_page() == 0) {
	        return Glib::filename_from_utf8(m_folder->get_current_folder());
	} else {
	       
		if (m_treeview->get_selection()->count_selected_rows() == 0) {
		        return Glib::filename_from_utf8(m_open_filechooser->get_current_folder());
		}
		Gtk::TreeModel::iterator i = m_treeview->get_selection()->get_selected();
		return (*i)[recent_columns.fullpath];
	}
}

bool
NewSessionDialog::use_session_template() const
{
        if(m_template->get_filename().empty() && (m_notebook->get_current_page() == 0)) return false;
	return true;
}

std::string
NewSessionDialog::session_template_name() const
{
	return Glib::filename_from_utf8(m_template->get_filename());
}

bool
NewSessionDialog::create_master_bus() const
{
	return m_create_master_bus->get_active();
}

int
NewSessionDialog::master_channel_count() const
{
	return m_master_bus_channel_count->get_value_as_int();
}

bool
NewSessionDialog::create_control_bus() const
{
	return m_create_control_bus->get_active();
}

int
NewSessionDialog::control_channel_count() const
{
	return m_control_bus_channel_count->get_value_as_int();
}

bool
NewSessionDialog::connect_inputs() const
{
	return m_connect_inputs->get_active();
}

bool
NewSessionDialog::limit_inputs_used_for_connection() const
{
	return m_limit_input_ports->get_active();
}

int
NewSessionDialog::input_limit_count() const
{
	return m_input_limit_count->get_value_as_int();
}

bool
NewSessionDialog::connect_outputs() const
{
	return m_connect_outputs->get_active();
}

bool
NewSessionDialog::limit_outputs_used_for_connection() const
{
	return m_limit_output_ports->get_active();
}

int
NewSessionDialog::output_limit_count() const
{
	return m_output_limit_count->get_value_as_int();
}

bool
NewSessionDialog::connect_outs_to_master() const
{
	return m_connect_outputs_to_master->get_active();
}

bool
NewSessionDialog::connect_outs_to_physical() const
{
	return m_connect_outputs_to_physical->get_active();
}

int
NewSessionDialog::get_current_page()
{
	return m_notebook->get_current_page();
	
}

void
NewSessionDialog::reset_name()
{
	m_name->set_text("");
	set_response_sensitive (Gtk::RESPONSE_OK, false);
	
}

bool
NewSessionDialog::entry_key_release (GdkEventKey* ev)
{
	if (m_name->get_text() != "") {
		set_response_sensitive (Gtk::RESPONSE_OK, true);
		set_response_sensitive (Gtk::RESPONSE_NONE, true);
	} else {
		set_response_sensitive (Gtk::RESPONSE_OK, false);
	}
	return true;
}

void
NewSessionDialog::notebook_page_changed (GtkNotebookPage* np, uint pagenum)
{
	if (pagenum == 1) {
		on_new_session_page = false;
		m_okbutton->set_label(_("Open"));
		set_response_sensitive (Gtk::RESPONSE_NONE, false);
		m_okbutton->set_image (*(new Gtk::Image (Gtk::Stock::OPEN, Gtk::ICON_SIZE_BUTTON)));
		if (m_treeview->get_selection()->count_selected_rows() == 0) {
			set_response_sensitive (Gtk::RESPONSE_OK, false);
		} else {
			set_response_sensitive (Gtk::RESPONSE_OK, true);
		}
	} else {
		on_new_session_page = true;
		if (m_name->get_text() != "") {
			set_response_sensitive (Gtk::RESPONSE_NONE, true);
		}
		m_okbutton->set_label(_("New"));
		m_okbutton->set_image (*(new Gtk::Image (Gtk::Stock::NEW, Gtk::ICON_SIZE_BUTTON)));
		if (m_name->get_text() == "") {
			set_response_sensitive (Gtk::RESPONSE_OK, false);
		} else {
			set_response_sensitive (Gtk::RESPONSE_OK, true);
		}
	}
}

void
NewSessionDialog::treeview_selection_changed ()
{
	if (m_treeview->get_selection()->count_selected_rows() == 0) {
		if (!m_open_filechooser->get_filename().empty()) {
			set_response_sensitive (Gtk::RESPONSE_OK, true);
		} else {
			set_response_sensitive (Gtk::RESPONSE_OK, false);
		}
	} else {
		set_response_sensitive (Gtk::RESPONSE_OK, true);
	}
}

void
NewSessionDialog::file_chosen ()
{
	if (on_new_session_page) return;

	m_treeview->get_selection()->unselect_all();

	if (!m_open_filechooser->get_filename().empty()) {
		set_response_sensitive (Gtk::RESPONSE_OK, true);
	} else {
		set_response_sensitive (Gtk::RESPONSE_OK, false);
	}
}

void
NewSessionDialog::template_chosen ()
{
	if (m_template->get_filename() != "" ) {;
		set_response_sensitive (Gtk::RESPONSE_NONE, true);
	} else {
		set_response_sensitive (Gtk::RESPONSE_NONE, false);
	}
}

void
NewSessionDialog::recent_row_activated (const Gtk::TreePath& path, Gtk::TreeViewColumn* col)
{
        response (Gtk::RESPONSE_YES);
}

void
NewSessionDialog::connect_inputs_clicked ()
{
        m_limit_input_ports->set_sensitive(m_connect_inputs->get_active());
}

void
NewSessionDialog::connect_outputs_clicked ()
{
        m_limit_output_ports->set_sensitive(m_connect_outputs->get_active());
}

void
NewSessionDialog::limit_inputs_clicked ()
{
        m_input_limit_count->set_sensitive(m_limit_input_ports->get_active());
}

void
NewSessionDialog::limit_outputs_clicked ()
{
        m_output_limit_count->set_sensitive(m_limit_output_ports->get_active());
}

void
NewSessionDialog::master_bus_button_clicked ()
{
        m_master_bus_channel_count->set_sensitive(m_create_master_bus->get_active());
}

void
NewSessionDialog::monitor_bus_button_clicked ()
{
        m_control_bus_channel_count->set_sensitive(m_create_control_bus->get_active());
}

void
NewSessionDialog::reset_template()
{
        m_template->set_filename("");
}

void
NewSessionDialog::reset_recent()
{
        /* Shamelessly ripped from ardour_ui.cc */
        std::vector<string *> *sessions;
	std::vector<string *>::iterator i;
	RecentSessionsSorter cmp;
	
	recent_model->clear ();
	
	ARDOUR::RecentSessions rs;
	ARDOUR::read_recent_sessions (rs);
	
	/* sort them alphabetically */
	sort (rs.begin(), rs.end(), cmp);
	sessions = new std::vector<std::string*>;
	
	for (ARDOUR::RecentSessions::iterator i = rs.begin(); i != rs.end(); ++i) {
	        sessions->push_back (new string ((*i).second));
	}
	
	for (i = sessions->begin(); i != sessions->end(); ++i) {

	        std::vector<std::string*>* states;
		std::vector<const gchar*> item;
		std::string fullpath = *(*i);
		
		/* remove any trailing / */
		
		if (fullpath[fullpath.length()-1] == '/') {
		        fullpath = fullpath.substr (0, fullpath.length()-1);
		}
	    
		/* now get available states for this session */
		  
		if ((states = ARDOUR::Session::possible_states (fullpath)) == 0) {
		        /* no state file? */
		        continue;
		}
	    
		Gtk::TreeModel::Row row = *(recent_model->append());
		
		row[recent_columns.visible_name] = Glib::path_get_basename (fullpath);
		row[recent_columns.fullpath] = fullpath;
		
		if (states->size() > 1) {
		    
		        /* add the children */
		    
		        for (std::vector<std::string*>::iterator i2 = states->begin(); i2 != states->end(); ++i2) {

			        Gtk::TreeModel::Row child_row = *(recent_model->append (row.children()));
				
				child_row[recent_columns.visible_name] = **i2;
				child_row[recent_columns.fullpath] = fullpath;
				
				delete *i2;
			}
		}

		delete states;
	}
	delete sessions;
}

void
NewSessionDialog::reset()
{
	reset_name();
	reset_template();
	set_response_sensitive (Gtk::RESPONSE_NONE, false);
}
