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

#include <utility>
#include <ardour/route.h>
#include <ardour/automation_control.h>
#include <pbd/memento_command.h>
#include <gtkmm2ext/barcontroller.h>

#include "ardour_ui.h"
#include "automation_time_axis.h"
#include "automation_line.h"
#include "public_editor.h"
#include "simplerect.h"
#include "selection.h"
#include "ghostregion.h"
#include "rgb_macros.h"
#include "automation_selectable.h"
#include "point_selection.h"
#include "canvas_impl.h"
#include "utils.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace Editing;

Pango::FontDescription AutomationTimeAxisView::name_font;
bool AutomationTimeAxisView::have_name_font = false;

AutomationTimeAxisView::AutomationTimeAxisView (Session& s, boost::shared_ptr<Route> r,
		boost::shared_ptr<Automatable> a, boost::shared_ptr<AutomationControl> c,
		PublicEditor& e, TimeAxisView& rent, 
		ArdourCanvas::Canvas& canvas, const string & nom, 
		const string & state_name, const string & nomparent)

	: AxisView (s), 
	  TimeAxisView (s, e, &rent, canvas),
	  _route (r),
	  _control (c),
	  _automatable (a),
	  _controller(AutomationController::create(s, c->list(), c)),
	  _base_rect (0),
	  _xml_node (0),
	  _name (nom),
	  _state_name (state_name),
	  height_button (_("h")),
	  clear_button (_("clear")),
	  auto_button (X_("")) /* force addition of a label */
{
	if (!have_name_font) {
		name_font = get_font_for_style (X_("AutomationTrackName"));
		have_name_font = true;
	}

	automation_menu = 0;
	in_destructor = false;
	auto_off_item = 0;
	auto_touch_item = 0;
	auto_write_item = 0;
	auto_play_item = 0;
	ignore_state_request = false;
	first_call_to_set_height = true;
	
	_base_rect = new SimpleRect(*canvas_display);
	_base_rect->property_x1() = 0.0;
	_base_rect->property_y1() = 0.0;
	_base_rect->property_x2() = editor.frame_to_pixel (max_frames);
	_base_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_AutomationTrackOutline.get();
	
	/* outline ends and bottom */
	_base_rect->property_outline_what() = (guint32) (0x1|0x2|0x8);
	_base_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_AutomationTrackFill.get();
	
	_base_rect->set_data ("trackview", this);

	_base_rect->signal_event().connect (bind (mem_fun (editor, &PublicEditor::canvas_automation_track_event),
						 _base_rect, this));

	hide_button.add (*(manage (new Gtk::Image (::get_icon("hide")))));

	height_button.set_name ("TrackSizeButton");
	auto_button.set_name ("TrackVisualButton");
	clear_button.set_name ("TrackVisualButton");
	hide_button.set_name ("TrackRemoveButton");

	controls_table.set_no_show_all();

	ARDOUR_UI::instance()->tooltips().set_tip(height_button, _("track height"));
	ARDOUR_UI::instance()->tooltips().set_tip(auto_button, _("automation state"));
	ARDOUR_UI::instance()->tooltips().set_tip(clear_button, _("clear track"));
	ARDOUR_UI::instance()->tooltips().set_tip(hide_button, _("hide track"));

	/* rearrange the name display */

	/* we never show these for automation tracks, so make
	   life easier and remove them.
	*/

	hide_name_entry();

	/* move the name label over a bit */

	string shortpname = _name;
	bool shortened = false;

	int ignore_width;
	shortpname = fit_to_pixels (_name, 60, name_font, ignore_width, true);

	if (shortpname != _name ){
		shortened = true;
	}

	name_label.set_text (shortpname);
	name_label.set_alignment (Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER);

	if (nomparent.length()) {

		/* limit the plug name string */

		string pname = fit_to_pixels (nomparent, 60, name_font, ignore_width, true);
		if (pname != nomparent) {
			shortened = true;
		}

 		plugname = new Label (pname);
		plugname->set_name (X_("TrackPlugName"));
		plugname->show();
		name_label.set_name (X_("TrackParameterName"));
		controls_table.remove (name_hbox);
		controls_table.attach (*plugname, 1, 5, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
		plugname_packed = true;
		controls_table.attach (name_hbox, 1, 5, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
	} else {
		plugname = 0;
		plugname_packed = false;
	}

	if (shortened) {
		string tipname = nomparent;
		if (!tipname.empty()) {
			tipname += ": ";
		}
		tipname += _name;
		ARDOUR_UI::instance()->tooltips().set_tip(controls_ebox, tipname);
	}
	
	/* add the buttons */
	controls_table.attach (hide_button, 0, 1, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
	controls_table.attach (height_button, 0, 1, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);

	controls_table.attach (auto_button, 5, 8, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
	controls_table.attach (clear_button, 5, 8, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
	
	/* add bar controller */
	controls_table.attach (*_controller.get(), 0, 8, 2, 3, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);

	controls_table.show_all ();

	height_button.signal_clicked().connect (mem_fun(*this, &AutomationTimeAxisView::height_clicked));
	clear_button.signal_clicked().connect (mem_fun(*this, &AutomationTimeAxisView::clear_clicked));
	hide_button.signal_clicked().connect (mem_fun(*this, &AutomationTimeAxisView::hide_clicked));
	auto_button.signal_clicked().connect (mem_fun(*this, &AutomationTimeAxisView::auto_clicked));

	controls_base_selected_name = X_("AutomationTrackControlsBaseSelected");
	controls_base_unselected_name = X_("AutomationTrackControlsBase");
	controls_ebox.set_name (controls_base_unselected_name);

	controls_frame.set_shadow_type (Gtk::SHADOW_ETCHED_OUT);

	XMLNode* xml_node = get_parent_with_state()->get_child_xml_node (_state_name);

	if (xml_node) {
		set_state (*xml_node);
	} 
		
	boost::shared_ptr<AutomationLine> line(new AutomationLine (
				_control->list()->parameter().to_string(),
				*this,
				*canvas_display,
				_control->list()));
		
	line->set_line_color (ARDOUR_UI::config()->canvasvar_ProcessorAutomationLine.get());
	line->queue_reset ();
	add_line (line);

	/* make sure labels etc. are correct */

	automation_state_changed ();
	ColorsChanged.connect (mem_fun (*this, &AutomationTimeAxisView::color_handler));
}

AutomationTimeAxisView::~AutomationTimeAxisView ()
{
	in_destructor = true;

	for (list<GhostRegion*>::iterator i = ghosts.begin(); i != ghosts.end(); ++i) {
		delete *i;
	}
}

void
AutomationTimeAxisView::auto_clicked ()
{
	using namespace Menu_Helpers;

	if (automation_menu == 0) {
		automation_menu = manage (new Menu);
		automation_menu->set_name ("ArdourContextMenu");
		MenuList& items (automation_menu->items());

		items.push_back (MenuElem (_("Manual"), 
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Off)));
		items.push_back (MenuElem (_("Play"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Play)));
		items.push_back (MenuElem (_("Write"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Write)));
		items.push_back (MenuElem (_("Touch"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Touch)));
	}

	automation_menu->popup (1, gtk_get_current_event_time());
}

void
AutomationTimeAxisView::set_automation_state (AutoState state)
{
	if (!ignore_state_request) {
		if (_route == _automatable) { // FIXME: ew
			_route->set_parameter_automation_state (
					_control->list()->parameter(),
					state);
		}

		_control->list()->set_automation_state(state);

	}
}

void
AutomationTimeAxisView::automation_state_changed ()
{
	AutoState state;

	/* update button label */

	if (!_line) {
		state = Off;
	} else {
		state = _control->list()->automation_state ();
	}

	switch (state & (Off|Play|Touch|Write)) {
	case Off:
		auto_button.set_label (_("Manual"));
		if (auto_off_item) {
			ignore_state_request = true;
			auto_off_item->set_active (true);
			auto_play_item->set_active (false);
			auto_touch_item->set_active (false);
			auto_write_item->set_active (false);
			ignore_state_request = false;
		}
		break;
	case Play:
		auto_button.set_label (_("Play"));
		if (auto_play_item) {
			ignore_state_request = true;
			auto_play_item->set_active (true);
			auto_off_item->set_active (false);
			auto_touch_item->set_active (false);
			auto_write_item->set_active (false);
			ignore_state_request = false;
		}
		break;
	case Write:
		auto_button.set_label (_("Write"));
		if (auto_write_item) {
			ignore_state_request = true;
			auto_write_item->set_active (true);
			auto_off_item->set_active (false);
			auto_play_item->set_active (false);
			auto_touch_item->set_active (false);
			ignore_state_request = false;
		}
		break;
	case Touch:
		auto_button.set_label (_("Touch"));
		if (auto_touch_item) {
			ignore_state_request = true;
			auto_touch_item->set_active (true);
			auto_off_item->set_active (false);
			auto_play_item->set_active (false);
			auto_write_item->set_active (false);
			ignore_state_request = false;
		}
		break;
	default:
		auto_button.set_label (_("???"));
		break;
	}
}

void
AutomationTimeAxisView::height_clicked ()
{
	popup_size_menu (0);
}

void
AutomationTimeAxisView::clear_clicked ()
{
	_session.begin_reversible_command (_("clear automation"));
	_line->clear ();
	_session.commit_reversible_command ();
}

void
AutomationTimeAxisView::set_height (TrackHeight ht)
{
	uint32_t h = height_to_pixels (ht);
	bool changed = (height != (uint32_t) h);

	//bool changed_between_small_and_normal = ( (ht == Small || ht == Smaller) ^ (height_style == Small || height_style == Smaller) );

	TimeAxisView* state_parent = get_parent_with_state ();
	XMLNode* xml_node = state_parent->get_child_xml_node (_state_name);

	TimeAxisView::set_height (ht);
	_base_rect->property_y2() = h;

	if (_line)
		_line->set_y_position_and_height (0, h);

	for (list<GhostRegion*>::iterator i = ghosts.begin(); i != ghosts.end(); ++i) {
		(*i)->set_height ();
	}


	switch (ht) {
	case Largest:
		xml_node->add_property ("track_height", "largest");
		break;

	case Large:
		xml_node->add_property ("track_height", "large");
		break;

	case Larger:
		xml_node->add_property ("track_height", "larger");
		break;

	case Normal:
		xml_node->add_property ("track_height", "normal");
		break;

	case Smaller:
		xml_node->add_property ("track_height", "smaller");
		break;

	case Small:
		xml_node->add_property ("track_height", "small");
		break;
	}

	//if (changed_between_small_and_normal || first_call_to_set_height) {
		first_call_to_set_height = false;
		switch (ht) {
			case Normal:
				_controller->hide();

				controls_table.remove (name_hbox);

				if (plugname) {
					if (plugname_packed) {
						controls_table.remove (*plugname);
						plugname_packed = false;
					}
					controls_table.attach (*plugname, 1, 5, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
					plugname_packed = true;
					controls_table.attach (name_hbox, 1, 5, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
				} else {
					controls_table.attach (name_hbox, 1, 5, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
				}
				hide_name_entry ();
				show_name_label ();
				name_hbox.show_all ();

				auto_button.show();
				height_button.show();
				clear_button.show();
				hide_button.show_all();
				break;
			
			case Large:
			case Larger:
			case Largest:
				_controller->show ();
				break;

			case Smaller:
				_controller->hide();
			
			case Small:

				controls_table.remove (name_hbox);
				if (plugname) {
					if (plugname_packed) {
						controls_table.remove (*plugname);
						plugname_packed = false;
					}
				}
				controls_table.attach (name_hbox, 1, 5, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL|Gtk::EXPAND);
				controls_table.hide_all ();
				hide_name_entry ();
				show_name_label ();
				name_hbox.show_all ();

				auto_button.hide();
				height_button.hide();
				clear_button.hide();
				hide_button.hide();
				break;
		}

	//}

	if (changed) {
		/* only emit the signal if the height really changed */
		_route->gui_changed ("track_height", (void *) 0); /* EMIT_SIGNAL */
	}
}

void
AutomationTimeAxisView::set_samples_per_unit (double spu)
{
	TimeAxisView::set_samples_per_unit (editor.get_current_zoom());

	_line->reset ();
}
 
void
AutomationTimeAxisView::hide_clicked ()
{
	// LAME fix for refreshing the hide button
	hide_button.set_sensitive(false);
	
	set_marked_for_display (false);
	hide ();
	
	hide_button.set_sensitive(true);
}

void
AutomationTimeAxisView::build_display_menu ()
{
	using namespace Menu_Helpers;

	/* get the size menu ready */

	build_size_menu ();

	/* prepare it */

	TimeAxisView::build_display_menu ();

	/* now fill it with our stuff */

	MenuList& items = display_menu->items();

	items.push_back (MenuElem (_("Height"), *size_menu));
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Hide"), mem_fun(*this, &AutomationTimeAxisView::hide_clicked)));
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Clear"), mem_fun(*this, &AutomationTimeAxisView::clear_clicked)));
	items.push_back (SeparatorElem());

	Menu* auto_state_menu = manage (new Menu);
	auto_state_menu->set_name ("ArdourContextMenu");
	MenuList& as_items = auto_state_menu->items();
	
	as_items.push_back (CheckMenuElem (_("Manual"), 
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Off)));
	auto_off_item = dynamic_cast<CheckMenuItem*>(&as_items.back());

	as_items.push_back (CheckMenuElem (_("Play"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Play)));
	auto_play_item = dynamic_cast<CheckMenuItem*>(&as_items.back());

	as_items.push_back (CheckMenuElem (_("Write"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Write)));
	auto_write_item = dynamic_cast<CheckMenuItem*>(&as_items.back());

	as_items.push_back (CheckMenuElem (_("Touch"),
					   bind (mem_fun(*this, &AutomationTimeAxisView::set_automation_state), (AutoState) Touch)));
	auto_touch_item = dynamic_cast<CheckMenuItem*>(&as_items.back());

	items.push_back (MenuElem (_("State"), *auto_state_menu));

	/* make sure the automation menu state is correct */

	automation_state_changed ();
}

void
AutomationTimeAxisView::add_automation_event (ArdourCanvas::Item* item, GdkEvent* event, nframes_t when, double y)
{
	double x = 0;

	canvas_display->w2i (x, y);

	/* compute vertical fractional position */

	y = 1.0 - (y / height);

	/* map using line */

	_line->view_to_model_y (y);

	_session.begin_reversible_command (_("add automation event"));
	XMLNode& before = _control->list()->get_state();

	_control->list()->add (when, y);

	XMLNode& after = _control->list()->get_state();
	_session.commit_reversible_command (new MementoCommand<ARDOUR::AutomationList>(*_control->list().get(), &before, &after));

	_session.set_dirty ();
}


bool
AutomationTimeAxisView::cut_copy_clear (Selection& selection, CutCopyOp op)
{
	return cut_copy_clear_one (*_line, selection, op);
}

bool
AutomationTimeAxisView::cut_copy_clear_one (AutomationLine& line, Selection& selection, CutCopyOp op)
{
	AutomationList* what_we_got = 0;
	boost::shared_ptr<AutomationList> alist (line.the_list());
	bool ret = false;

	XMLNode &before = alist->get_state();

	switch (op) {
	case Cut:
		if ((what_we_got = alist->cut (selection.time.front().start, selection.time.front().end)) != 0) {
			editor.get_cut_buffer().add (what_we_got);
			_session.add_command(new MementoCommand<AutomationList>(*alist.get(), &before, &alist->get_state()));
			ret = true;
		}
		break;
	case Copy:
		if ((what_we_got = alist->copy (selection.time.front().start, selection.time.front().end)) != 0) {
			editor.get_cut_buffer().add (what_we_got);
		}
		break;

	case Clear:
		if ((what_we_got = alist->cut (selection.time.front().start, selection.time.front().end)) != 0) {
			_session.add_command(new MementoCommand<AutomationList>(*alist.get(), &before, &alist->get_state()));
			delete what_we_got;
			what_we_got = 0;
			ret = true;
		}
		break;
	}

	if (what_we_got) {
		for (AutomationList::iterator x = what_we_got->begin(); x != what_we_got->end(); ++x) {
			double foo = (*x)->value;
			line.model_to_view_y (foo);
			(*x)->value = foo;
		}
	}

	return ret;
}

void
AutomationTimeAxisView::reset_objects (PointSelection& selection)
{
	reset_objects_one (*_line, selection);
}

void
AutomationTimeAxisView::reset_objects_one (AutomationLine& line, PointSelection& selection)
{
	boost::shared_ptr<AutomationList> alist(line.the_list());

	_session.add_command (new MementoCommand<AutomationList>(*alist.get(), &alist->get_state(), 0));

	for (PointSelection::iterator i = selection.begin(); i != selection.end(); ++i) {

		if (&(*i).track != this) {
			continue;
		}
		
		alist->reset_range ((*i).start, (*i).end);
	}
}

bool
AutomationTimeAxisView::cut_copy_clear_objects (PointSelection& selection, CutCopyOp op)
{
	return cut_copy_clear_objects_one (*_line, selection, op);
}

bool
AutomationTimeAxisView::cut_copy_clear_objects_one (AutomationLine& line, PointSelection& selection, CutCopyOp op)
{
	AutomationList* what_we_got = 0;
	boost::shared_ptr<AutomationList> alist(line.the_list());
	bool ret = false;

	XMLNode &before = alist->get_state();

	for (PointSelection::iterator i = selection.begin(); i != selection.end(); ++i) {

		if (&(*i).track != this) {
			continue;
		}

		switch (op) {
		case Cut:
			if ((what_we_got = alist->cut ((*i).start, (*i).end)) != 0) {
				editor.get_cut_buffer().add (what_we_got);
				_session.add_command (new MementoCommand<AutomationList>(*alist.get(), new XMLNode (before), &alist->get_state()));
				ret = true;
			}
			break;
		case Copy:
			if ((what_we_got = alist->copy ((*i).start, (*i).end)) != 0) {
				editor.get_cut_buffer().add (what_we_got);
			}
			break;
			
		case Clear:
			if ((what_we_got = alist->cut ((*i).start, (*i).end)) != 0) {
				_session.add_command (new MementoCommand<AutomationList>(*alist.get(), new XMLNode (before), &alist->get_state()));
				delete what_we_got;
				what_we_got = 0;
				ret = true;
			}
			break;
		}
	}

	delete &before;

	if (what_we_got) {
		for (AutomationList::iterator x = what_we_got->begin(); x != what_we_got->end(); ++x) {
			double foo = (*x)->value;
			line.model_to_view_y (foo);
			(*x)->value = foo;
		}
	}

	return ret;
}

bool
AutomationTimeAxisView::paste (nframes_t pos, float times, Selection& selection, size_t nth)
{
	return paste_one (*_line, pos, times, selection, nth);
}

bool
AutomationTimeAxisView::paste_one (AutomationLine& line, nframes_t pos, float times, Selection& selection, size_t nth)
{
	AutomationSelection::iterator p;
	boost::shared_ptr<AutomationList> alist(line.the_list());
	
	for (p = selection.lines.begin(); p != selection.lines.end() && nth; ++p, --nth);

	if (p == selection.lines.end()) {
		return false;
	}

	/* Make a copy of the list because we have to scale the
	   values from view coordinates to model coordinates, and we're
	   not supposed to modify the points in the selection.
	*/
	   
	AutomationList copy (**p);

	for (AutomationList::iterator x = copy.begin(); x != copy.end(); ++x) {
		double foo = (*x)->value;
		line.view_to_model_y (foo);
		(*x)->value = foo;
	}

	XMLNode &before = alist->get_state();
	alist->paste (copy, pos, times);
	_session.add_command (new MementoCommand<AutomationList>(*alist.get(), &before, &alist->get_state()));

	return true;
}

void
AutomationTimeAxisView::add_ghost (GhostRegion* gr)
{
	ghosts.push_back (gr);
	gr->GoingAway.connect (mem_fun(*this, &AutomationTimeAxisView::remove_ghost));
}

void
AutomationTimeAxisView::remove_ghost (GhostRegion* gr)
{
	if (in_destructor) {
		return;
	}

	list<GhostRegion*>::iterator i;

	for (i = ghosts.begin(); i != ghosts.end(); ++i) {
		if ((*i) == gr) {
			ghosts.erase (i);
			break;
		}
	}
}

void
AutomationTimeAxisView::get_selectables (nframes_t start, nframes_t end, double top, double bot, list<Selectable*>& results)
{
	if (_line && touched (top, bot)) {
		double topfrac;
		double botfrac;

		/* remember: this is X Window - coordinate space starts in upper left and moves down.
		   y_position is the "origin" or "top" of the track.
		*/

		double mybot = y_position + height;

		if (y_position >= top && mybot <= bot) {

			/* y_position is below top, mybot is above bot, so we're fully
			   covered vertically.
			*/

			topfrac = 1.0;
			botfrac = 0.0;

		} else {

			/* top and bot are within y_position .. mybot */

			topfrac = 1.0 - ((top - y_position) / height);
			botfrac = 1.0 - ((bot - y_position) / height);
		}

		_line->get_selectables (start, end, botfrac, topfrac, results);
	}
}

void
AutomationTimeAxisView::get_inverted_selectables (Selection& sel, list<Selectable*>& result)
{
	_line->get_inverted_selectables (sel, result);
}

void
AutomationTimeAxisView::set_selected_points (PointSelection& points)
{
	_line->set_selected_points (points);
}

void
AutomationTimeAxisView::clear_lines ()
{
	_line.reset();
	automation_connection.disconnect ();
}

void
AutomationTimeAxisView::add_line (boost::shared_ptr<AutomationLine> line)
{
	assert(line);
	assert(!_line);
	assert(line->the_list() == _control->list());

	automation_connection = _control->list()->automation_state_changed.connect
		(mem_fun(*this, &AutomationTimeAxisView::automation_state_changed));

	_line = line;
	//_controller = AutomationController::create(_session, line->the_list(), _control);

	line->set_y_position_and_height (0, height);

	/* pick up the current state */
	automation_state_changed ();

	line->show();
}

void
AutomationTimeAxisView::show_all_control_points ()
{
	_line->show_all_control_points ();
}

void
AutomationTimeAxisView::hide_all_but_selected_control_points ()
{
	_line->hide_all_but_selected_control_points ();
}

void
AutomationTimeAxisView::entered()
{
	show_all_control_points ();
}

void
AutomationTimeAxisView::exited ()
{
	hide_all_but_selected_control_points ();
}

void
AutomationTimeAxisView::set_colors ()
{
    for (list<GhostRegion*>::iterator i=ghosts.begin(); i != ghosts.end(); i++ ) {
		(*i)->set_colors();
    }
    
	_line->set_colors();
}

void
AutomationTimeAxisView::color_handler () 
{
	set_colors ();
}

void
AutomationTimeAxisView::set_state (const XMLNode& node)
{
	TimeAxisView::set_state (node);
	
	XMLNodeList kids;
	XMLNodeConstIterator iter;

	kids = node.children ();

	//snprintf (buf, sizeof(buf), "Port_%" PRIu32, param.id());
		
	for (iter = kids.begin(); iter != kids.end(); ++iter) {
		if ((*iter)->name() == _control->list()->parameter().to_string()) {
		
			XMLProperty *shown = (*iter)->property("shown_editor");
			
			if (shown && shown->value() == "yes") {
				set_marked_for_display(true);
				canvas_display->show(); /* FIXME: necessary? show_at? */
			}
			break;
		}
	}

	if (!_marked_for_display)
		hide();

	// FIXME: _xml_node = &node?
}

XMLNode*
AutomationTimeAxisView::get_state_node ()
{
	TimeAxisView* state_parent = get_parent_with_state ();

	if (state_parent) {
		return state_parent->get_child_xml_node (_state_name);
	} else {
		return 0;
	}
}

void
AutomationTimeAxisView::ensure_xml_node ()
{
	if ((_automatable != _route) && _xml_node == 0) {
		if ((_xml_node = _automatable->extra_xml ("GUI")) == 0) {
			_xml_node = new XMLNode ("GUI");
			_automatable->add_extra_xml (*_xml_node);
		}
	}
}

void
AutomationTimeAxisView::update_extra_xml_shown (bool editor_shown)
{
	if (_automatable == _route)
		return;

	char buf[32];
	
	ensure_xml_node ();

	XMLNodeList nlist = _xml_node->children ();
	XMLNodeConstIterator i;
	XMLNode * port_node = 0;

	/* FIXME: these parsed XML node names need to go */
	//snprintf (buf, sizeof(buf), "Port_%" PRIu32, _param.id());

	for (i = nlist.begin(); i != nlist.end(); ++i) {
		/* FIXME: legacy session loading */
		if ((*i)->name() == _control->list()->parameter().to_string()) {
			port_node = (*i);
			break;
		}
	}

	if (!port_node) {
		port_node = new XMLNode(buf);
		_xml_node->add_child_nocopy(*port_node);
	}
	
	port_node->add_property ("shown_editor", editor_shown ? "yes": "no");
}

guint32
AutomationTimeAxisView::show_at (double y, int& nth, Gtk::VBox *parent)
{
	update_extra_xml_shown (true);
	
	return TimeAxisView::show_at (y, nth, parent);
}

void
AutomationTimeAxisView::hide ()
{
	update_extra_xml_shown (false);

	TimeAxisView::hide ();
}

