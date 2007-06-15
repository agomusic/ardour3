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

#ifndef __ardour_gtk_automation_time_axis_h__
#define __ardour_gtk_automation_time_axis_h__

#include <vector>
#include <list>
#include <string>

#include <boost/shared_ptr.hpp>

#include <ardour/types.h>

#include "canvas.h"
#include "time_axis_view.h"
#include "simplerect.h"

using std::vector;
using std::list;
using std::string;

namespace ARDOUR {
	class Session;
	class Route;
}

class PublicEditor;
class TimeSelection;
class RegionSelection;
class PointSelection;
class AutomationLine;
class GhostRegion;
class Selection;
class Selectable;

class AutomationTimeAxisView : public TimeAxisView {
  public:
	AutomationTimeAxisView (ARDOUR::Session&,
				boost::shared_ptr<ARDOUR::Route>,
				PublicEditor&,
				TimeAxisView& parent,
				ArdourCanvas::Canvas& canvas,
				const string & name, /* translatable */
				const string & state_name, /* not translatable */
				const string & plug_name = "");

	~AutomationTimeAxisView();
	
	virtual void set_height (TimeAxisView::TrackHeight);
	void set_samples_per_unit (double);
	std::string name() const { return _name; }

	virtual void add_automation_event (ArdourCanvas::Item *item, GdkEvent *event, nframes_t, double) = 0;

	virtual void clear_lines ();
	virtual void add_line (AutomationLine&);

	vector<AutomationLine*> lines;

	void set_selected_points (PointSelection&);
	void get_selectables (nframes_t start, nframes_t end, double top, double bot, list<Selectable *>&);
	void get_inverted_selectables (Selection&, list<Selectable*>& results);

	void show_timestretch (nframes_t start, nframes_t end) {}
	void hide_timestretch () {}

	/* editing operations */
	
	bool cut_copy_clear (Selection&, Editing::CutCopyOp);
	bool cut_copy_clear_objects (PointSelection&, Editing::CutCopyOp);
	bool paste (nframes_t, float times, Selection&, size_t nth);
	void reset_objects (PointSelection&);

	void add_ghost (GhostRegion*);
	void remove_ghost (GhostRegion*);

	void show_all_control_points ();
	void hide_all_but_selected_control_points ();
	void set_state (const XMLNode&);
	XMLNode* get_state_node ();

  protected:
	boost::shared_ptr<ARDOUR::Route> route;
	ArdourCanvas::SimpleRect* base_rect;
	string _name;
	string _state_name;
	bool    in_destructor;

	bool    first_call_to_set_height;

	Gtk::Button        hide_button;
	Gtk::Button        height_button;
	Gtk::Button        clear_button;
	Gtk::Button        auto_button; 
	Gtk::Menu*         automation_menu;
	Gtk::Label*        plugname;
	bool               plugname_packed;

	Gtk::CheckMenuItem*     auto_off_item;
	Gtk::CheckMenuItem*     auto_play_item;
	Gtk::CheckMenuItem*     auto_touch_item;
	Gtk::CheckMenuItem*     auto_write_item;

	void clear_clicked ();
	void height_clicked ();
	void hide_clicked ();
	void auto_clicked ();

	virtual void build_display_menu ();

	list<GhostRegion*> ghosts;

	bool cut_copy_clear_one (AutomationLine&, Selection&, Editing::CutCopyOp);
	bool cut_copy_clear_objects_one (AutomationLine&, PointSelection&, Editing::CutCopyOp);
	bool paste_one (AutomationLine&, nframes_t, float times, Selection&, size_t nth);
	void reset_objects_one (AutomationLine&, PointSelection&);

	virtual void set_automation_state (ARDOUR::AutoState) = 0;
	bool ignore_state_request;

	void automation_state_changed ();
	sigc::connection automation_connection;

	void entered ();
	void exited ();

	void set_colors ();
	void color_handler ();

	static Pango::FontDescription name_font;
	static bool have_name_font;
};

#endif /* __ardour_gtk_automation_time_axis_h__ */
