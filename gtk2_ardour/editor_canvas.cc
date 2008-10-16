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

*/

#include <libgnomecanvasmm/init.h>
#include <jack/types.h>
#include <gtkmm2ext/utils.h>

#include <ardour/audioregion.h>
#include <ardour/profile.h>

#include "ardour_ui.h"
#include "editor.h"
#include "waveview.h"
#include "simplerect.h"
#include "simpleline.h"
#include "waveview_p.h"
#include "simplerect_p.h"
#include "simpleline_p.h"
#include "canvas_impl.h"
#include "editing.h"
#include "rgb_macros.h"
#include "utils.h"
#include "time_axis_view.h"
#include "audio_time_axis.h"

#ifdef WITH_CMT
#include "imageframe.h"
#include "imageframe_p.h"
#endif

#include "i18n.h"

using namespace std;
using namespace sigc;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace Editing;

/* XXX this is a hack. it ought to be the maximum value of an nframes64_t */

const double max_canvas_coordinate = (double) JACK_MAX_FRAMES;

extern "C"
{

GType gnome_canvas_simpleline_get_type(void);
GType gnome_canvas_simplerect_get_type(void);
GType gnome_canvas_waveview_get_type(void);

#ifdef WITH_CMT
GType gnome_canvas_imageframe_get_type(void);
#endif

}

static void ardour_canvas_type_init() 
{
	// Map gtypes to gtkmm wrapper-creation functions:
	
	Glib::wrap_register(gnome_canvas_simpleline_get_type(), &Gnome::Canvas::SimpleLine_Class::wrap_new);
	Glib::wrap_register(gnome_canvas_simplerect_get_type(), &Gnome::Canvas::SimpleRect_Class::wrap_new);
	Glib::wrap_register(gnome_canvas_waveview_get_type(), &Gnome::Canvas::WaveView_Class::wrap_new);

#ifdef WITH_CMT
	Glib::wrap_register(gnome_canvas_imageframe_get_type(), &Gnome::Canvas::ImageFrame_Class::wrap_new);
#endif
	
	// Register the gtkmm gtypes:

	(void) Gnome::Canvas::WaveView::get_type();
	(void) Gnome::Canvas::SimpleLine::get_type();
	(void) Gnome::Canvas::SimpleRect::get_type();
	
#ifdef WITH_CMT
	(void) Gnome::Canvas::ImageFrame::get_type();
#endif
} 


void
Editor::initialize_canvas ()
{
	if (getenv ("ARDOUR_NON_AA_CANVAS")) {
		track_canvas = new ArdourCanvas::Canvas ();
	} else {
		track_canvas = new ArdourCanvas::CanvasAA ();
	}
	
	ArdourCanvas::init ();
	ardour_canvas_type_init ();

	/* don't try to center the canvas */

	track_canvas->set_center_scroll_region (false);
	track_canvas->set_dither (Gdk::RGB_DITHER_NONE);

	Glib::RefPtr<Gdk::Screen> screen = get_screen();
	
	if (!screen) {
		screen = Gdk::Screen::get_default();
	}
	physical_screen_width = screen->get_width ();
	physical_screen_height = screen->get_height ();

	/* stuff for the verbose canvas cursor */

	Pango::FontDescription* font = get_font_for_style (N_("VerboseCanvasCursor"));

	verbose_canvas_cursor = new ArdourCanvas::Text (*track_canvas->root());
	verbose_canvas_cursor->property_font_desc() = *font;
	verbose_canvas_cursor->property_anchor() = ANCHOR_NW;
	
	delete font;

	verbose_cursor_visible = false;
	
	if (Profile->get_sae()) {
		Image img (::get_icon (X_("saelogo")));
		logo_item = new ArdourCanvas::Pixbuf (*track_canvas->root(), 0.0, 0.0, img.get_pixbuf());
		// logo_item->property_height_in_pixels() = true;
		// logo_item->property_width_in_pixels() = true;
		// logo_item->property_height_set() = true;
		// logo_item->property_width_set() = true;
		logo_item->show ();
	}

	_master_group = new ArdourCanvas::Group (*track_canvas->root(), 0.0, 0.0);

	transport_loop_range_rect = new ArdourCanvas::SimpleRect (*_master_group, 0.0, 0.0, 0.0, physical_screen_width);
	transport_loop_range_rect->property_outline_pixels() = 1;
	transport_loop_range_rect->hide();

	transport_punch_range_rect = new ArdourCanvas::SimpleRect (*_master_group, 0.0, 0.0, 0.0, physical_screen_width);
	transport_punch_range_rect->property_outline_pixels() = 0;
	transport_punch_range_rect->hide();

	/* a group to hold time (measure) lines */
	time_line_group = new ArdourCanvas::Group (*_master_group, 0.0, 0.0);

	range_marker_drag_rect = new ArdourCanvas::SimpleRect (*time_line_group, 0.0, 0.0, 0.0, physical_screen_width);
	range_marker_drag_rect->hide ();

	_trackview_group = new ArdourCanvas::Group (*_master_group, 0.0, 0.0);
	_region_motion_group = new ArdourCanvas::Group (*_trackview_group, 0.0, 0.0);

	/* el barrio */

	meter_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	meter_bar = new ArdourCanvas::SimpleRect (*meter_bar_group, 0.0, 0.0, physical_screen_width, timebar_height-1.0);
	meter_bar->property_outline_what() = (0x1 | 0x8);
	meter_bar->property_outline_pixels() = 1;

	tempo_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	tempo_bar = new ArdourCanvas::SimpleRect (*tempo_bar_group, 0.0, 0.0, physical_screen_width, (timebar_height-1.0));
	tempo_bar->property_outline_what() = (0x1 | 0x8);
	tempo_bar->property_outline_pixels() = 1;

	range_marker_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	range_marker_bar = new ArdourCanvas::SimpleRect (*range_marker_bar_group, 0.0, 0.0, physical_screen_width, (timebar_height-1.0));
	range_marker_bar->property_outline_what() = (0x1 | 0x8);
	range_marker_bar->property_outline_pixels() = 1;
	
	transport_marker_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	transport_marker_bar = new ArdourCanvas::SimpleRect (*transport_marker_bar_group, 0.0, 0.0, physical_screen_width, (timebar_height-1.0));
	transport_marker_bar->property_outline_what() = (0x1 | 0x8);
	transport_marker_bar->property_outline_pixels() = 1;

	marker_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	marker_bar = new ArdourCanvas::SimpleRect (*marker_bar_group, 0.0, 0.0, physical_screen_width, (timebar_height-1.0));
	marker_bar->property_outline_what() = (0x1 | 0x8);
	marker_bar->property_outline_pixels() = 1;
	
	cd_marker_bar_group = new ArdourCanvas::Group (*track_canvas->root());
	cd_marker_bar = new ArdourCanvas::SimpleRect (*cd_marker_bar_group, 0.0, 0.0, physical_screen_width, (timebar_height-1.0));
 	cd_marker_bar->property_outline_what() = (0x1 | 0x8);
 	cd_marker_bar->property_outline_pixels() = 1;

	timebar_group =  new ArdourCanvas::Group (*track_canvas->root());
	cursor_group = new ArdourCanvas::Group (*track_canvas->root(), 0.0, 0.0);

	meter_group = new ArdourCanvas::Group (*timebar_group, 0.0, timebar_height * 5.0);
	tempo_group = new ArdourCanvas::Group (*timebar_group, 0.0, timebar_height * 4.0);
	range_marker_group = new ArdourCanvas::Group (*timebar_group, 0.0, timebar_height * 3.0);
	transport_marker_group = new ArdourCanvas::Group (*timebar_group, 0.0, timebar_height * 2.0);
	marker_group = new ArdourCanvas::Group (*timebar_group, 0.0, timebar_height);
	cd_marker_group = new ArdourCanvas::Group (*timebar_group, 0.0, 0.0);

	marker_drag_line_points.push_back(Gnome::Art::Point(0.0, 0.0));
	marker_drag_line_points.push_back(Gnome::Art::Point(0.0, 1.0));

	marker_drag_line = new ArdourCanvas::Line (*timebar_group);
	marker_drag_line->property_width_pixels() = 1;
	marker_drag_line->property_points() = marker_drag_line_points;
	marker_drag_line->hide();

	cd_marker_bar_drag_rect = new ArdourCanvas::SimpleRect (*cd_marker_group, 0.0, 0.0, 100, timebar_height);
	cd_marker_bar_drag_rect->property_outline_pixels() = 0;
	cd_marker_bar_drag_rect->hide ();

	range_bar_drag_rect = new ArdourCanvas::SimpleRect (*range_marker_group, 0.0, 0.0, 100, timebar_height);
	range_bar_drag_rect->property_outline_pixels() = 0;
	range_bar_drag_rect->hide ();

	transport_bar_drag_rect = new ArdourCanvas::SimpleRect (*transport_marker_group, 0.0, 0.0, 100, timebar_height);
	transport_bar_drag_rect->property_outline_pixels() = 0;
	transport_bar_drag_rect->hide ();

	transport_punchin_line = new ArdourCanvas::SimpleLine (*_master_group);
	transport_punchin_line->property_x1() = 0.0;
	transport_punchin_line->property_y1() = 0.0;
	transport_punchin_line->property_x2() = 0.0;
	transport_punchin_line->property_y2() = physical_screen_width;
	transport_punchin_line->hide ();
	
	transport_punchout_line = new ArdourCanvas::SimpleLine (*_master_group);
	transport_punchout_line->property_x1() = 0.0;
	transport_punchout_line->property_y1() = 0.0;
	transport_punchout_line->property_x2() = 0.0;
	transport_punchout_line->property_y2() = physical_screen_width;
	transport_punchout_line->hide();
	
	// used to show zoom mode active zooming
	zoom_rect = new ArdourCanvas::SimpleRect (*_master_group, 0.0, 0.0, 0.0, 0.0);
	zoom_rect->property_outline_pixels() = 1;
	zoom_rect->hide();
	
	zoom_rect->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_zoom_rect_event), (ArdourCanvas::Item*) 0));
	
	// used as rubberband rect
	rubberband_rect = new ArdourCanvas::SimpleRect (*_trackview_group, 0.0, 0.0, 0.0, 0.0);

	rubberband_rect->property_outline_pixels() = 1;
	rubberband_rect->hide();
	
	tempo_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_tempo_bar_event), tempo_bar));
	meter_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_meter_bar_event), meter_bar));
	marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_marker_bar_event), marker_bar));
	cd_marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_cd_marker_bar_event), cd_marker_bar));
	range_marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_range_marker_bar_event), range_marker_bar));
	transport_marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_transport_marker_bar_event), transport_marker_bar));

	playhead_cursor = new Cursor (*this, &Editor::canvas_playhead_cursor_event);

	if (logo_item) {
		logo_item->lower_to_bottom ();
	}

	/* need to handle 4 specific types of events as catch-alls */

	track_canvas->signal_scroll_event().connect (mem_fun (*this, &Editor::track_canvas_scroll_event));
	track_canvas->signal_motion_notify_event().connect (mem_fun (*this, &Editor::track_canvas_motion_notify_event));
	track_canvas->signal_button_press_event().connect (mem_fun (*this, &Editor::track_canvas_button_press_event));
	track_canvas->signal_button_release_event().connect (mem_fun (*this, &Editor::track_canvas_button_release_event));

	track_canvas->set_name ("EditorMainCanvas");
	track_canvas->add_events (Gdk::POINTER_MOTION_HINT_MASK|Gdk::SCROLL_MASK);
	track_canvas->signal_leave_notify_event().connect (mem_fun(*this, &Editor::left_track_canvas));
	track_canvas->signal_enter_notify_event().connect (mem_fun(*this, &Editor::entered_track_canvas));
	track_canvas->set_flags (CAN_FOCUS);

	/* set up drag-n-drop */

	vector<TargetEntry> target_table;
	
	// Drag-N-Drop from the region list can generate this target
	target_table.push_back (TargetEntry ("regions"));
	target_table.push_back (TargetEntry ("routes"));

	target_table.push_back (TargetEntry ("text/plain"));
	target_table.push_back (TargetEntry ("text/uri-list"));
	target_table.push_back (TargetEntry ("application/x-rootwin-drop"));

	track_canvas->drag_dest_set (target_table);
	track_canvas->signal_drag_data_received().connect (mem_fun(*this, &Editor::track_canvas_drag_data_received));

	track_canvas->signal_size_allocate().connect (mem_fun(*this, &Editor::track_canvas_allocate));

	ColorsChanged.connect (mem_fun (*this, &Editor::color_handler));
	color_handler();

}

void
Editor::track_canvas_allocate (Gtk::Allocation alloc)
{
	canvas_allocation = alloc;
	track_canvas_size_allocated ();
}

bool
Editor::track_canvas_size_allocated ()
{
	bool height_changed = canvas_height != canvas_allocation.get_height();

	canvas_width = canvas_allocation.get_width();
	canvas_height = canvas_allocation.get_height();

	if (session) {
		TrackViewList::iterator i;
		double height = 0;

		for (i = track_views.begin(); i != track_views.end(); ++i) {
			if ((*i)->control_parent) {
				height += (*i)->effective_height;
			}
			(*i)->clip_to_viewport ();
		}
		
		full_canvas_height = height + canvas_timebars_vsize;
	} else {
		return true;
	}

	if (height_changed) {
		if (playhead_cursor) {
			playhead_cursor->set_length (canvas_height);
		}
		
		vertical_adjustment.set_page_size (canvas_height);
		
		for (MarkerSelection::iterator x = selection->markers.begin(); x != selection->markers.end(); ++x) {
			(*x)->set_line_vpos (0, canvas_height);
		}
		
	}

	handle_new_duration ();
	reset_hscrollbar_stepping ();
	update_fixed_rulers();
	redisplay_tempo (false);

	last_trackview_group_vertical_offset = get_trackview_group_vertical_offset ();
	if ((vertical_adjustment.get_value() + canvas_height) >= vertical_adjustment.get_upper()) {
		/* 
		   We're increasing the size of the canvas while the bottom is visible.
		   We scroll down to keep in step with the controls layout.
		*/
		vertical_adjustment.set_value (full_canvas_height - canvas_height + 1);
	} 
	
	Resized (); /* EMIT_SIGNAL */
		
	return false;
}

void
Editor::controls_layout_size_request (Requisition* req)
{
	TreeModel::Children rows = route_display_model->children();
	TreeModel::Children::iterator i;
	double pos;
	bool changed = false;

	for (pos = 0, i = rows.begin(); i != rows.end(); ++i) {
		TimeAxisView *tv = (*i)[route_display_columns.tv];
		if (tv != 0) {
			pos += tv->effective_height;
			tv->clip_to_viewport ();
		}
	}

	gint height = min ( (gint) pos, (gint) (physical_screen_height - 600));
	gint width = max (edit_controls_vbox.get_width(),  controls_layout.get_width());

	/* don't get too big. the fudge factors here are just guesses */

	width =  min (width, (gint) (physical_screen_width - 300));

	if ((req->width != width) || (req->height != height)) {
		changed = true;
		controls_layout_size_request_connection.disconnect ();
	}

	if (req->width != width) {
		gint vbox_width = edit_controls_vbox.get_width();
		req->width = width;

		/* this one is important: it determines how big the layout thinks it really is, as 
		   opposed to what it displays on the screen
		*/
		controls_layout.property_width () = vbox_width;
		controls_layout.property_width_request () = vbox_width;

		time_button_event_box.property_width_request () = vbox_width;
		zoom_box.property_width_request () = vbox_width;
	}

	if (req->height != height) {
		req->height = height;
		controls_layout.property_height () = (guint) floor (pos);
	}

	if (changed) {
		controls_layout_size_request_connection = controls_layout.signal_size_request().connect (mem_fun (*this, &Editor::controls_layout_size_request));
	}
}

bool
Editor::track_canvas_map_handler (GdkEventAny* ev)
{
	track_canvas->get_window()->set_cursor (*current_canvas_cursor);
	return false;
}

void  
Editor::track_canvas_drag_data_received (const RefPtr<Gdk::DragContext>& context,
					 int x, int y, 
					 const SelectionData& data,
					 guint info, guint time)
{
	cerr << "drop on canvas, target = " << data.get_target() << endl;

	if (data.get_target() == "regions") {
		drop_regions (context, x, y, data, info, time);
	}
	else if(data.get_target() == "routes") {
		drop_routes (context, x, y, data, info, time);
	}
	else {
		drop_paths (context, x, y, data, info, time);
	}
}

void
Editor::drop_paths (const RefPtr<Gdk::DragContext>& context,
		    int x, int y, 
		    const SelectionData& data,
		    guint info, guint time)
{
	TimeAxisView* tvp;
	RouteTimeAxisView* tv;
	double cy;
	vector<ustring> paths;
	string spath;
	GdkEvent ev;
	nframes64_t frame;

	if (convert_drop_to_paths (paths, context, x, y, data, info, time)) {
		goto out;
	}

	/* D-n-D coordinates are window-relative, so convert to "world" coordinates
	 */

	double wx;
	double wy;

	track_canvas->window_to_world (x, y, wx, wy);
	//wx += horizontal_adjustment.get_value();
	//wy += vertical_adjustment.get_value();
	
	ev.type = GDK_BUTTON_RELEASE;
	ev.button.x = wx;
	ev.button.y = wy;

	frame = event_frame (&ev, 0, &cy);

	snap_to (frame);

	if ((tvp = trackview_by_y_position (cy)) == 0) {

		/* drop onto canvas background: create new tracks */

		frame = 0;

		if (Profile->get_sae() || Config->get_only_copy_imported_files()) {
			do_import (paths, Editing::ImportDistinctFiles, Editing::ImportAsTrack, SrcBest, frame); 
		} else {
			do_embed (paths, Editing::ImportDistinctFiles, ImportAsTrack, frame);
		}
		
	} else if ((tv = dynamic_cast<RouteTimeAxisView*>(tvp)) != 0) {

		/* check that its an audio track, not a bus */
		
		/* check that its an audio track, not a bus */
		
		if (tv->get_diskstream()) {
			/* select the track, then embed */
			selection->set (tv);

			if (Profile->get_sae() || Config->get_only_copy_imported_files()) {
				do_import (paths, Editing::ImportDistinctFiles, Editing::ImportToTrack, SrcBest, frame); 
			} else {
				do_embed (paths, Editing::ImportDistinctFiles, ImportToTrack, frame);
			}
		}
	}

  out:
	context->drag_finish (true, false, time);
}

void
Editor::drop_regions (const RefPtr<Gdk::DragContext>& context,
		      int x, int y, 
		      const SelectionData& data,
		      guint info, guint time)
{
	const SerializedObjectPointers<boost::shared_ptr<Region> >* sr = 
		reinterpret_cast<const SerializedObjectPointers<boost::shared_ptr<Region> > *> (data.get_data());

	for (uint32_t i = 0; i < sr->cnt; ++i) {

		boost::shared_ptr<Region> r = sr->data[i];
		
		insert_region_list_drag (r, x, y);
	}

	context->drag_finish (true, false, time);
}

void
Editor::drop_routes (const Glib::RefPtr<Gdk::DragContext>& context,
		     int x, int y,
		     const Gtk::SelectionData& data,
		     guint info, guint time) {
	const SerializedObjectPointers<boost::shared_ptr<Route> >* sr = 
		reinterpret_cast<const SerializedObjectPointers<boost::shared_ptr<Route> > *> (data.get_data());

	for (uint32_t i = 0; i < sr->cnt; ++i) {
		boost::shared_ptr<Route> r = sr->data[i];
		insert_route_list_drag (r, x, y);
	}

	context->drag_finish (true, false, time);
}

void
Editor::maybe_autoscroll (GdkEventMotion* event)
{

	nframes64_t rightmost_frame = leftmost_frame + current_page_frames();
	nframes64_t frame = drag_info.current_pointer_frame;
	bool startit = false;

	autoscroll_y = 0;
	autoscroll_x = 0;
	if (event->y < canvas_timebars_vsize) {
		autoscroll_y = -1;
		startit = true;
	} else if (event->y > canvas_height) {
		autoscroll_y = 1;
		startit = true;
	}

	if (frame > rightmost_frame) {

		if (rightmost_frame < max_frames) {
			autoscroll_x = 1;
			startit = true;
		}

	} else if (frame < leftmost_frame) {
		if (leftmost_frame > 0) {
			autoscroll_x = -1;
			startit = true;
		}

	}

	if (!allow_vertical_scroll) {
		autoscroll_y = 0;
	}

	if ((autoscroll_x != last_autoscroll_x) || (autoscroll_y != last_autoscroll_y) || (autoscroll_x == 0 && autoscroll_y == 0)) {
		stop_canvas_autoscroll ();
	}
	
	if (startit && autoscroll_timeout_tag < 0) {
		start_canvas_autoscroll (autoscroll_x, autoscroll_y);
	}

	last_autoscroll_x = autoscroll_x;
	last_autoscroll_y = autoscroll_y;
}

void
Editor::maybe_autoscroll_horizontally (GdkEventMotion* event)
{
	nframes64_t rightmost_frame = leftmost_frame + current_page_frames();
	nframes64_t frame = drag_info.current_pointer_frame;
	bool startit = false;

	autoscroll_y = 0;
	autoscroll_x = 0;

	if (frame > rightmost_frame) {

		if (rightmost_frame < max_frames) {
			autoscroll_x = 1;
			startit = true;
		}

	} else if (frame < leftmost_frame) {
		if (leftmost_frame > 0) {
			autoscroll_x = -1;
			startit = true;
		}

	}

	if ((autoscroll_x != last_autoscroll_x) || (autoscroll_y != last_autoscroll_y) || (autoscroll_x == 0 && autoscroll_y == 0)) {
		stop_canvas_autoscroll ();
	}
	
	if (startit && autoscroll_timeout_tag < 0) {
		start_canvas_autoscroll (autoscroll_x, autoscroll_y);
	}

	last_autoscroll_x = autoscroll_x;
	last_autoscroll_y = autoscroll_y;
}

gint
Editor::_autoscroll_canvas (void *arg)
{
        return ((Editor *) arg)->autoscroll_canvas ();
}

bool
Editor::autoscroll_canvas ()
{
	nframes64_t new_frame;
	nframes64_t limit = max_frames - current_page_frames();
	GdkEventMotion ev;
	double new_pixel;
	double target_pixel;

	if (autoscroll_x > 0) {
		autoscroll_x_distance = (unit_to_frame (drag_info.current_pointer_x) - (leftmost_frame + current_page_frames())) / 3;
	} else if (autoscroll_x < 0) {
		autoscroll_x_distance = (leftmost_frame - unit_to_frame (drag_info.current_pointer_x)) / 3;
		
	}
	
	if (autoscroll_y > 0) {
		autoscroll_y_distance = (drag_info.current_pointer_y - (get_trackview_group_vertical_offset() + canvas_height)) / 3;
	} else if (autoscroll_y < 0) {
		
		autoscroll_y_distance = (vertical_adjustment.get_value () - drag_info.current_pointer_y) / 3;
	}
	
	if (autoscroll_x < 0) {
		if (leftmost_frame < autoscroll_x_distance) {
			new_frame = 0;
		} else {
			new_frame = leftmost_frame - autoscroll_x_distance;
		}
 	} else if (autoscroll_x > 0) {
		if (leftmost_frame > limit - autoscroll_x_distance) {
			new_frame = limit;
		} else {
			new_frame = leftmost_frame + autoscroll_x_distance;
		}
	} else {
		new_frame = leftmost_frame;
	}

	double vertical_pos = vertical_adjustment.get_value();

	if (autoscroll_y < 0) {

		if (vertical_pos < autoscroll_y_distance) {
			new_pixel = 0;
		} else {
			new_pixel = vertical_pos - autoscroll_y_distance;
		}

		target_pixel = drag_info.current_pointer_y - autoscroll_y_distance;
		target_pixel = max (target_pixel, 0.0);

 	} else if (autoscroll_y > 0) {

		double top_of_bottom_of_canvas = full_canvas_height - canvas_height;

		if (vertical_pos > full_canvas_height - autoscroll_y_distance) {
			new_pixel = full_canvas_height;
		} else {
			new_pixel = vertical_pos + autoscroll_y_distance;
		}

		new_pixel = min (top_of_bottom_of_canvas, new_pixel);

		target_pixel = drag_info.current_pointer_y + autoscroll_y_distance;
		
		/* don't move to the full canvas height because the item will be invisible
		   (its top edge will line up with the bottom of the visible canvas.
		*/

		target_pixel = min (target_pixel, full_canvas_height - 10);
		
	} else {
	  	target_pixel = drag_info.current_pointer_y;
		new_pixel = vertical_pos;
	}

	if ((new_frame == 0 || new_frame == limit) && (new_pixel == 0 || new_pixel == DBL_MAX)) {
		/* we are done */
		return false;
	}

	if (new_frame != leftmost_frame) {
		reset_x_origin (new_frame);
	}

	/* fake an event. */

	Glib::RefPtr<Gdk::Window> canvas_window = const_cast<Editor*>(this)->track_canvas->get_window();
	gint x, y;
	Gdk::ModifierType mask;
	canvas_window->get_pointer (x, y, mask);
	ev.type = GDK_MOTION_NOTIFY;
	ev.state &= Gdk::BUTTON1_MASK;
	ev.x = x;
	ev.y = y;

	motion_handler (drag_info.item, (GdkEvent*) &ev, drag_info.item_type, true);

	vertical_adjustment.set_value (new_pixel);

	autoscroll_cnt++;

	if (autoscroll_cnt == 1) {

		/* connect the timeout so that we get called repeatedly */

		autoscroll_timeout_tag = g_idle_add ( _autoscroll_canvas, this);
		return false;

	} 

	return true;
}

void
Editor::start_canvas_autoscroll (int dx, int dy)
{
	if (!session || autoscroll_active) {
		return;
	}

	stop_canvas_autoscroll ();

	autoscroll_active = true;
	autoscroll_x = dx;
	autoscroll_y = dy;
	autoscroll_x_distance = (nframes64_t) floor (current_page_frames()/50.0);
	autoscroll_y_distance = fabs (dy * 5); /* pixels */
	autoscroll_cnt = 0;
	
	/* do it right now, which will start the repeated callbacks */

	autoscroll_canvas ();
}

void
Editor::stop_canvas_autoscroll ()
{

	if (autoscroll_timeout_tag >= 0) {
		g_source_remove (autoscroll_timeout_tag);
		autoscroll_timeout_tag = -1;
	}

	autoscroll_active = false;
}

bool
Editor::left_track_canvas (GdkEventCrossing *ev)
{
	set_entered_track (0);
	set_entered_regionview (0);
	reset_canvas_action_sensitivity (false);
	return false;
}

bool
Editor::entered_track_canvas (GdkEventCrossing *ev)
{
	reset_canvas_action_sensitivity (true);
	return FALSE;
}

void
Editor::tie_vertical_scrolling ()
{
	scroll_canvas_vertically ();

	/* this will do an immediate redraw */

	controls_layout.get_vadjustment()->set_value (vertical_adjustment.get_value());
}

void
Editor::scroll_canvas_horizontally ()
{
	nframes64_t time_origin = (nframes64_t) floor (horizontal_adjustment.get_value() * frames_per_unit);

	if (time_origin != leftmost_frame) {
		canvas_scroll_to (time_origin);
	}

	/* horizontal scrolling only */
	double x1, x2, y1, y2, x_delta;

	_master_group->get_bounds(x1, y1, x2, y2);
	x_delta = x1 + horizontal_adjustment.get_value();

	_master_group->move (-x_delta, 0);
	timebar_group->move (-x_delta, 0);
	cursor_group->move (-x_delta, 0);
	update_fixed_rulers ();
	redisplay_tempo (true);
}

void
Editor::scroll_canvas_vertically ()
{
	/* vertical scrolling only */
	double y_delta;

	y_delta = last_trackview_group_vertical_offset - get_trackview_group_vertical_offset ();

	_trackview_group->move (0, y_delta);

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(*i)->clip_to_viewport ();
	}
	last_trackview_group_vertical_offset = get_trackview_group_vertical_offset ();
	/* required to keep the controls_layout in lock step with the canvas group */
	track_canvas->update_now ();
}

void 
Editor::canvas_horizontally_scrolled ()
{
	nframes64_t time_origin = (nframes64_t) floor (horizontal_adjustment.get_value() * frames_per_unit);

	if (time_origin != leftmost_frame) {
		canvas_scroll_to (time_origin);
	}
	redisplay_tempo (true);
}

void
Editor::canvas_scroll_to (nframes64_t time_origin)
{
	leftmost_frame = time_origin;
	nframes64_t rightmost_frame = leftmost_frame + current_page_frames ();

	if (rightmost_frame > last_canvas_frame) {
		last_canvas_frame = rightmost_frame;
		//reset_scrolling_region ();
	}
}

void
Editor::color_handler()
{
	playhead_cursor->canvas_item.property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_PlayHead.get();
	verbose_canvas_cursor->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_VerboseCanvasCursor.get();
	
	meter_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_MeterBar.get();
	meter_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	tempo_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_TempoBar.get();
	tempo_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	marker_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBar.get();
	marker_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	cd_marker_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_CDMarkerBar.get();
	cd_marker_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	range_marker_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeMarkerBar.get();
	range_marker_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	transport_marker_bar->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportMarkerBar.get();
	transport_marker_bar->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerBarSeparator.get();

	cd_marker_bar_drag_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragBarRect.get();
	cd_marker_bar_drag_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragBarRect.get();

	range_bar_drag_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragBarRect.get();
	range_bar_drag_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragBarRect.get();

	transport_bar_drag_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportDragRect.get();
	transport_bar_drag_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportDragRect.get();

	marker_drag_line->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_MarkerDragLine.get();

	range_marker_drag_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragRect.get();
	range_marker_drag_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_RangeDragRect.get();

	transport_loop_range_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportLoopRect.get();
	transport_loop_range_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportLoopRect.get();

	transport_punch_range_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportPunchRect.get();
	transport_punch_range_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_TransportPunchRect.get();

	transport_punchin_line->property_color_rgba() = ARDOUR_UI::config()->canvasvar_PunchLine.get();
	transport_punchout_line->property_color_rgba() = ARDOUR_UI::config()->canvasvar_PunchLine.get();

	zoom_rect->property_fill_color_rgba() = ARDOUR_UI::config()->canvasvar_ZoomRect.get();
	zoom_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_ZoomRect.get();

	rubberband_rect->property_outline_color_rgba() = ARDOUR_UI::config()->canvasvar_RubberBandRect.get();
	rubberband_rect->property_fill_color_rgba() = (guint32) ARDOUR_UI::config()->canvasvar_RubberBandRect.get();

	location_marker_color = ARDOUR_UI::config()->canvasvar_LocationMarker.get();
	location_range_color = ARDOUR_UI::config()->canvasvar_LocationRange.get();
	location_cd_marker_color = ARDOUR_UI::config()->canvasvar_LocationCDMarker.get();
	location_loop_color = ARDOUR_UI::config()->canvasvar_LocationLoop.get();
	location_punch_color = ARDOUR_UI::config()->canvasvar_LocationPunch.get();

	refresh_location_display ();
/*
	redisplay_tempo (true);

	if (session)
		session->tempo_map().apply_with_metrics (*this, &Editor::draw_metric_marks); // redraw metric markers
*/
}

void
Editor::flush_canvas ()
{
	if (is_mapped()) {
		track_canvas->update_now ();
		// gdk_window_process_updates (GTK_LAYOUT(track_canvas->gobj())->bin_window, true);
	}
}
