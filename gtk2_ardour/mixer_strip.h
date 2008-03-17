/*
    Copyright (C) 2000-2006 Paul Davis

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

#ifndef __ardour_mixer_strip__
#define __ardour_mixer_strip__

#include <vector>

#include <cmath>

#include <gtkmm/eventbox.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/menu.h>
#include <gtkmm/textview.h>
#include <gtkmm/adjustment.h>

#include <gtkmm2ext/auto_spin.h>
#include <gtkmm2ext/click_box.h>
#include <gtkmm2ext/slider_controller.h>

#include <pbd/stateful.h>

#include <ardour/types.h>
#include <ardour/ardour.h>
#include <ardour/io.h>
#include <ardour/processor.h>
#include <ardour/io_processor.h>

#include <pbd/fastlog.h>

#include "route_ui.h"
#include "io_selector.h"
#include "gain_meter.h"
#include "panner_ui.h"
#include "enums.h"
#include "processor_box.h"
#include "ardour_dialog.h"

class MotionController;


namespace Gtkmm2ext {
	class SliderController;
}

namespace ARDOUR {
	class Route;
	class Send;
	class Processor;
	class Session;
	class PortInsert;
	class Bundle;
	class Plugin;
}
namespace Gtk {
	class Window;
	class Style;
}

class Mixer_UI;

class MixerStrip : public RouteUI, public Gtk::EventBox
{
  public:
	MixerStrip (Mixer_UI&, ARDOUR::Session&, boost::shared_ptr<ARDOUR::Route>, bool in_mixer = true);
	~MixerStrip ();

	void set_width (Width, void* owner);
	Width get_width() const { return _width; }
	void* width_owner() const { return _width_owner; }

	void fast_update ();
	void set_embedded (bool);
	
	ARDOUR::RouteGroup* mix_group() const;

  protected:
	friend class Mixer_UI;
	void set_packed (bool yn);
	bool packed () { return _packed; }

	void set_selected(bool yn);
	void set_stuff_from_route ();

  private:
	Mixer_UI& _mixer;

	bool  _embedded;
	bool  _packed;
	bool  _mixer_owned;
	Width _width;
	void*  _width_owner;

	Gtk::Button         hide_button;
	Gtk::Button         width_button;
	Gtk::HBox           width_hide_box;
	Gtk::EventBox       top_event_box;

	void hide_clicked();
	void width_clicked ();

	Gtk::Frame          global_frame;
	Gtk::VBox           global_vpacker;

	ProcessorBox pre_processor_box;
	ProcessorBox post_processor_box;
	GainMeter   gpm;
	PannerUI    panners;
	
	Gtk::Table button_table;
	Gtk::Table middle_button_table;
	Gtk::Table bottom_button_table;

	Gtk::Button                  gain_unit_button;
	Gtk::Label                   gain_unit_label;
	Gtk::Button                  meter_point_button;
	Gtk::Label                   meter_point_label;

	void meter_changed (void *);

	Gtk::Button diskstream_button;
	Gtk::Label  diskstream_label;

	Gtk::Button input_button;
	Gtk::Label  input_label;
	Gtk::Button output_button;
	Gtk::Label  output_label;

	sigc::connection newplug_connection;
    
	gint    mark_update_safe ();
	guint32 mode_switch_in_progress;
	
	Gtk::Button   name_button;

	ArdourDialog*  comment_window;
	Gtk::TextView* comment_area;
	Gtk::Button    comment_button;

	void comment_editor_done_editing();
	void setup_comment_editor ();
	void comment_button_clicked ();

	Gtk::Button   group_button;
	Gtk::Label    group_label;
	Gtk::Menu    *group_menu;

	gint input_press (GdkEventButton *);
	gint output_press (GdkEventButton *);

	Gtk::Menu  input_menu;
	void add_bundle_to_input_menu (boost::shared_ptr<ARDOUR::Bundle>, std::vector<boost::shared_ptr<ARDOUR::Bundle> > const &);

	Gtk::Menu output_menu;
	void add_bundle_to_output_menu (boost::shared_ptr<ARDOUR::Bundle>, std::vector<boost::shared_ptr<ARDOUR::Bundle> > const &);
	
	void bundle_input_chosen (boost::shared_ptr<ARDOUR::Bundle>);
	void bundle_output_chosen (boost::shared_ptr<ARDOUR::Bundle>);

	void edit_input_configuration ();
	void edit_output_configuration ();

	void diskstream_changed ();

	Gtk::Menu *send_action_menu;
	void build_send_action_menu ();

	void new_send ();
	void show_send_controls ();

	void input_changed (ARDOUR::IOChange, void *);
	void output_changed (ARDOUR::IOChange, void *);

	sigc::connection panstate_connection;
	sigc::connection panstyle_connection;
	void connect_to_pan ();

	void update_diskstream_display ();
	void update_input_display ();
	void update_output_display ();

	void set_automated_controls_sensitivity (bool yn);

	Gtk::Menu* route_ops_menu;
	void build_route_ops_menu ();
	gint name_button_button_press (GdkEventButton*);
	void list_route_operations ();

	gint comment_key_release_handler (GdkEventKey*);
	void comment_changed (void *src);
	void comment_edited ();
	bool ignore_comment_edit;

	void set_mix_group (ARDOUR::RouteGroup *);
	void add_mix_group_to_menu (ARDOUR::RouteGroup *, Gtk::RadioMenuItem::Group*);
	bool select_mix_group (GdkEventButton *);
	void mix_group_changed (void *);

	IOSelectorWindow *input_selector;
	IOSelectorWindow *output_selector;

	Gtk::Style *passthru_style;

	void route_gui_changed (string, void*);
	void show_route_color ();
	void show_passthru_color ();

	void route_active_changed ();

	/* speed control (for tracks only) */

	Gtk::Adjustment    speed_adjustment;
	Gtkmm2ext::ClickBox speed_spinner;
	Gtk::Label         speed_label;
	Gtk::Frame         speed_frame;

	void speed_adjustment_changed ();
	void speed_changed ();
	void name_changed ();
	void update_speed_display ();
	void map_frozen ();
	void hide_processor_editor (boost::shared_ptr<ARDOUR::Processor> processor);
	void hide_redirect_editors ();

	bool ignore_speed_adjustment;

	void engine_running();
	void engine_stopped();

	static int scrollbar_height;
};

#endif /* __ardour_mixer_strip__ */
