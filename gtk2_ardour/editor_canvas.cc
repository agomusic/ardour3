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

#include "ardour_ui.h"
#include "editor.h"
#include "waveview.h"
#include "simplerect.h"
#include "simpleline.h"
#include "imageframe.h"
#include "waveview_p.h"
#include "simplerect_p.h"
#include "simpleline_p.h"
#include "imageframe_p.h"
#include "canvas_impl.h"
#include "editing.h"
#include "rgb_macros.h"
#include "utils.h"
#include "time_axis_view.h"
#include "audio_time_axis.h"

#include "i18n.h"

using namespace std;
using namespace sigc;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace Editing;

/* XXX this is a hack. it ought to be the maximum value of an nframes_t */

const double max_canvas_coordinate = (double) JACK_MAX_FRAMES;

extern "C"
{

GType gnome_canvas_simpleline_get_type(void);
GType gnome_canvas_simplerect_get_type(void);
GType gnome_canvas_waveview_get_type(void);
GType gnome_canvas_imageframe_get_type(void);

}

static void ardour_canvas_type_init() 
{
	// Map gtypes to gtkmm wrapper-creation functions:
	
	Glib::wrap_register(gnome_canvas_simpleline_get_type(), &Gnome::Canvas::SimpleLine_Class::wrap_new);
	Glib::wrap_register(gnome_canvas_simplerect_get_type(), &Gnome::Canvas::SimpleRect_Class::wrap_new);
	Glib::wrap_register(gnome_canvas_waveview_get_type(), &Gnome::Canvas::WaveView_Class::wrap_new);
	Glib::wrap_register(gnome_canvas_imageframe_get_type(), &Gnome::Canvas::ImageFrame_Class::wrap_new);
	
	// Register the gtkmm gtypes:

	(void) Gnome::Canvas::WaveView::get_type();
	(void) Gnome::Canvas::SimpleLine::get_type();
	(void) Gnome::Canvas::SimpleRect::get_type();
	(void) Gnome::Canvas::ImageFrame::get_type();
} 

void
Editor::initialize_canvas ()
{
	ArdourCanvas::init ();
	ardour_canvas_type_init ();

	/* don't try to center the canvas */

	track_canvas.set_center_scroll_region (false);
	track_canvas.set_dither  	(Gdk::RGB_DITHER_NONE);

	track_canvas.signal_event().connect (bind (mem_fun (*this, &Editor::track_canvas_event), (ArdourCanvas::Item*) 0));
	track_canvas.set_name ("EditorMainCanvas");
	track_canvas.add_events (Gdk::POINTER_MOTION_HINT_MASK|Gdk::SCROLL_MASK);
	track_canvas.signal_leave_notify_event().connect (mem_fun(*this, &Editor::left_track_canvas));
	track_canvas.set_flags (CAN_FOCUS);

	/* set up drag-n-drop */

	vector<TargetEntry> target_table;
	
	// Drag-N-Drop from the region list can generate this target
	target_table.push_back (TargetEntry ("regions"));

	target_table.push_back (TargetEntry ("text/plain"));
	target_table.push_back (TargetEntry ("text/uri-list"));
	target_table.push_back (TargetEntry ("application/x-rootwin-drop"));

	track_canvas.drag_dest_set (target_table);
	track_canvas.signal_drag_data_received().connect (mem_fun(*this, &Editor::track_canvas_drag_data_received));

	/* stuff for the verbose canvas cursor */

	Pango::FontDescription font = get_font_for_style (N_("VerboseCanvasCursor"));

	verbose_canvas_cursor = new ArdourCanvas::Text (*track_canvas.root());
	verbose_canvas_cursor->property_font_desc() = font;
	verbose_canvas_cursor->property_anchor() = ANCHOR_NW;
	verbose_canvas_cursor->property_fill_color_rgba() = color_map[cVerboseCanvasCursor];
	
	verbose_cursor_visible = false;
	
	/* a group to hold time (measure) lines */
	
	time_line_group = new ArdourCanvas::Group (*track_canvas.root(), 0.0, 0.0);
	cursor_group = new ArdourCanvas::Group (*track_canvas.root(), 0.0, 0.0);

	time_canvas.set_name ("EditorTimeCanvas");
	time_canvas.add_events (Gdk::POINTER_MOTION_HINT_MASK);
	time_canvas.set_flags (CAN_FOCUS);
	time_canvas.set_center_scroll_region (false);
	time_canvas.set_dither  	(Gdk::RGB_DITHER_NONE);
	
	meter_group = new ArdourCanvas::Group (*time_canvas.root(), 0.0, 0.0);
	tempo_group = new ArdourCanvas::Group (*time_canvas.root(), 0.0, timebar_height);
	marker_group = new ArdourCanvas::Group (*time_canvas.root(), 0.0, timebar_height * 2.0);
	range_marker_group = new ArdourCanvas::Group (*time_canvas.root(), 0.0, timebar_height * 3.0);
	transport_marker_group = new ArdourCanvas::Group (*time_canvas.root(), 0.0, timebar_height * 4.0);
	
	tempo_bar = new ArdourCanvas::SimpleRect (*tempo_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	tempo_bar->property_fill_color_rgba() = color_map[cTempoBar];
	tempo_bar->property_outline_pixels() = 0;
	
	meter_bar = new ArdourCanvas::SimpleRect (*meter_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	meter_bar->property_fill_color_rgba() = color_map[cMeterBar];
	meter_bar->property_outline_pixels() = 0;
	
	marker_bar = new ArdourCanvas::SimpleRect (*marker_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	marker_bar->property_fill_color_rgba() = color_map[cMarkerBar];
	marker_bar->property_outline_pixels() = 0;
	
	range_marker_bar = new ArdourCanvas::SimpleRect (*range_marker_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	range_marker_bar->property_fill_color_rgba() = color_map[cRangeMarkerBar];
	range_marker_bar->property_outline_pixels() = 0;
	
	transport_marker_bar = new ArdourCanvas::SimpleRect (*transport_marker_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	transport_marker_bar->property_fill_color_rgba() = color_map[cTransportMarkerBar];
	transport_marker_bar->property_outline_pixels() = 0;
	
	range_bar_drag_rect = new ArdourCanvas::SimpleRect (*range_marker_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	range_bar_drag_rect->property_fill_color_rgba() = color_map[cRangeDragBarRectFill];
	range_bar_drag_rect->property_outline_color_rgba() = color_map[cRangeDragBarRect];
	range_bar_drag_rect->property_outline_pixels() = 0;
	range_bar_drag_rect->hide ();
	
	transport_bar_drag_rect = new ArdourCanvas::SimpleRect (*transport_marker_group, 0.0, 0.0, max_canvas_coordinate, timebar_height);
	transport_bar_drag_rect ->property_fill_color_rgba() = color_map[cTransportDragRectFill];
	transport_bar_drag_rect->property_outline_color_rgba() = color_map[cTransportDragRect];
	transport_bar_drag_rect->property_outline_pixels() = 0;
	transport_bar_drag_rect->hide ();
	
	marker_drag_line_points.push_back(Gnome::Art::Point(0.0, 0.0));
	marker_drag_line_points.push_back(Gnome::Art::Point(0.0, 0.0));

	marker_drag_line = new ArdourCanvas::Line (*track_canvas.root());
	marker_drag_line->property_width_pixels() = 1;
	marker_drag_line->property_fill_color_rgba() = color_map[cMarkerDragLine];
	marker_drag_line->property_points() = marker_drag_line_points;
	marker_drag_line->hide();

	range_marker_drag_rect = new ArdourCanvas::SimpleRect (*track_canvas.root(), 0.0, 0.0, 0.0, 0.0);
	range_marker_drag_rect->property_fill_color_rgba() = color_map[cRangeDragRectFill];
	range_marker_drag_rect->property_outline_color_rgba() = color_map[cRangeDragRect];
	range_marker_drag_rect->hide ();
	
	transport_loop_range_rect = new ArdourCanvas::SimpleRect (*time_line_group, 0.0, 0.0, 0.0, 0.0);
	transport_loop_range_rect->property_fill_color_rgba() = color_map[cTransportLoopRectFill];
	transport_loop_range_rect->property_outline_color_rgba() = color_map[cTransportLoopRect];
	transport_loop_range_rect->property_outline_pixels() = 1;
	transport_loop_range_rect->hide();

	transport_punch_range_rect = new ArdourCanvas::SimpleRect (*time_line_group, 0.0, 0.0, 0.0, 0.0);
	transport_punch_range_rect->property_fill_color_rgba() = color_map[cTransportPunchRectFill];
	transport_punch_range_rect->property_outline_color_rgba() = color_map[cTransportPunchRect];
	transport_punch_range_rect->property_outline_pixels() = 0;
	transport_punch_range_rect->hide();
	
	transport_loop_range_rect->lower_to_bottom (); // loop on the bottom

	transport_punchin_line = new ArdourCanvas::SimpleLine (*time_line_group);
	transport_punchin_line->property_x1() = 0.0;
	transport_punchin_line->property_y1() = 0.0;
	transport_punchin_line->property_x2() = 0.0;
	transport_punchin_line->property_y2() = 0.0;
	transport_punchin_line->property_color_rgba() = color_map[cPunchInLine];
	transport_punchin_line->hide ();
	
	transport_punchout_line  = new ArdourCanvas::SimpleLine (*time_line_group);
	transport_punchout_line->property_x1() = 0.0;
	transport_punchout_line->property_y1() = 0.0;
	transport_punchout_line->property_x2() = 0.0;
	transport_punchout_line->property_y2() = 0.0;
	transport_punchout_line->property_color_rgba() = color_map[cPunchOutLine];
	transport_punchout_line->hide();
	
	// used to show zoom mode active zooming
	zoom_rect = new ArdourCanvas::SimpleRect (*track_canvas.root(), 0.0, 0.0, 0.0, 0.0);
	zoom_rect->property_fill_color_rgba() = color_map[cZoomRectFill];
	zoom_rect->property_outline_color_rgba() = color_map[cZoomRect];
	zoom_rect->property_outline_pixels() = 1;
	zoom_rect->hide();
	
	zoom_rect->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_zoom_rect_event), (ArdourCanvas::Item*) 0));
	
	// used as rubberband rect
	rubberband_rect = new ArdourCanvas::SimpleRect (*track_canvas.root(), 0.0, 0.0, 0.0, 0.0);
	rubberband_rect->property_outline_color_rgba() = color_map[cRubberBandRect];
	rubberband_rect->property_fill_color_rgba() = (guint32) color_map[cRubberBandRectFill];
	rubberband_rect->property_outline_pixels() = 1;
	rubberband_rect->hide();
	
	tempo_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_tempo_bar_event), tempo_bar));
	meter_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_meter_bar_event), meter_bar));
	marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_marker_bar_event), marker_bar));
	range_marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_range_marker_bar_event), range_marker_bar));
	transport_marker_bar->signal_event().connect (bind (mem_fun (*this, &Editor::canvas_transport_marker_bar_event), transport_marker_bar));
	
	/* separator lines */
	
	tempo_line = new ArdourCanvas::SimpleLine (*tempo_group, 0, timebar_height, max_canvas_coordinate, timebar_height);
	tempo_line->property_color_rgba() = RGBA_TO_UINT (0,0,0,255);

	meter_line = new ArdourCanvas::SimpleLine (*meter_group, 0, timebar_height, max_canvas_coordinate, timebar_height);
	meter_line->property_color_rgba() = RGBA_TO_UINT (0,0,0,255);

	marker_line = new ArdourCanvas::SimpleLine (*marker_group, 0, timebar_height, max_canvas_coordinate, timebar_height);
	marker_line->property_color_rgba() = RGBA_TO_UINT (0,0,0,255);
	
	range_marker_line = new ArdourCanvas::SimpleLine (*range_marker_group, 0, timebar_height, max_canvas_coordinate, timebar_height);
	range_marker_line->property_color_rgba() = RGBA_TO_UINT (0,0,0,255);

	transport_marker_line = new ArdourCanvas::SimpleLine (*transport_marker_group, 0, timebar_height, max_canvas_coordinate, timebar_height);
	transport_marker_line->property_color_rgba() = RGBA_TO_UINT (0,0,0,255);

	ZoomChanged.connect (bind (mem_fun(*this, &Editor::update_loop_range_view), false));
	ZoomChanged.connect (bind (mem_fun(*this, &Editor::update_punch_range_view), false));
	
	double time_height = timebar_height * 5;
	double time_width = FLT_MAX/frames_per_unit;
	time_canvas.set_scroll_region(0.0, 0.0, time_width, time_height);
	
	edit_cursor = new Cursor (*this, &Editor::canvas_edit_cursor_event);
	edit_cursor->canvas_item.property_fill_color_rgba() = color_map[cEditCursor];
	playhead_cursor = new Cursor (*this, &Editor::canvas_playhead_cursor_event);
	playhead_cursor->canvas_item.property_fill_color_rgba() = color_map[cPlayHead];

	initial_ruler_update_required = true;
	track_canvas.signal_size_allocate().connect (mem_fun(*this, &Editor::track_canvas_allocate));

}

void
Editor::track_canvas_allocate (Gtk::Allocation alloc)
{
	canvas_allocation = alloc;

	if (!initial_ruler_update_required) {
		if (!canvas_idle_queued) {
			/* call this first so that we do stuff before any pending redraw */
			Glib::signal_idle().connect (mem_fun (*this, &Editor::track_canvas_size_allocated), false);
			canvas_idle_queued = true;
		}
		return;
	} 

	initial_ruler_update_required = false;
	
	track_canvas_size_allocated ();
}

bool
Editor::track_canvas_size_allocated ()
{
	if (canvas_idle_queued) {
		canvas_idle_queued = false;
	}

	canvas_width = canvas_allocation.get_width();
	canvas_height = canvas_allocation.get_height();

	full_canvas_height = canvas_height;

	if (session) {
		TrackViewList::iterator i;
		double height = 0;

		for (i = track_views.begin(); i != track_views.end(); ++i) {
			if ((*i)->control_parent) {
				height += (*i)->effective_height;
				height += track_spacing;
			}
		}
		
		if (height) {
			height -= track_spacing;
		}

		full_canvas_height = height;
	}

	zoom_range_clock.set ((nframes_t) floor ((canvas_width * frames_per_unit)));
	edit_cursor->set_position (edit_cursor->current_frame);
	playhead_cursor->set_position (playhead_cursor->current_frame);

	reset_hscrollbar_stepping ();
	reset_scrolling_region ();

	if (edit_cursor) edit_cursor->set_length (canvas_height);
	if (playhead_cursor) playhead_cursor->set_length (canvas_height);

	if (marker_drag_line) {
		marker_drag_line_points.back().set_y(canvas_height);
		marker_drag_line->property_points() = marker_drag_line_points;
	}

	if (range_marker_drag_rect) {
		range_marker_drag_rect->property_y1() = 0.0;
		range_marker_drag_rect->property_y2() = canvas_height;
	}

	if (transport_loop_range_rect) {
		transport_loop_range_rect->property_y1() = 0.0;
		transport_loop_range_rect->property_y2() = canvas_height;
	}

	if (transport_punch_range_rect) {
		transport_punch_range_rect->property_y1() = 0.0;
		transport_punch_range_rect->property_y2() = canvas_height;
	}

	if (transport_punchin_line) {
		transport_punchin_line->property_y1() = 0.0;
		transport_punchin_line->property_y2() = canvas_height;
	}

	if (transport_punchout_line) {
		transport_punchout_line->property_y1() = 0.0;
		transport_punchout_line->property_y2() = canvas_height;
	}
		
	update_fixed_rulers();
	tempo_map_changed (Change (0), true);
	
	Resized (); /* EMIT_SIGNAL */

	return false;
}

void
Editor::reset_scrolling_region (Gtk::Allocation* alloc)
{
	TreeModel::Children rows = route_display_model->children();
	TreeModel::Children::iterator i;
	double pos;

        for (pos = 0, i = rows.begin(); i != rows.end(); ++i) {
	        TimeAxisView *tv = (*i)[route_display_columns.tv];
		if (tv != 0 && !tv->hidden()) {
			pos += tv->effective_height;
			pos += track_spacing;
		}
	}

	double last_canvas_unit =  last_canvas_frame / frames_per_unit;

	track_canvas.set_scroll_region (0.0, 0.0, max (last_canvas_unit, canvas_width), pos);

	// XXX what is the correct height value for the time canvas ? this overstates it
	time_canvas.set_scroll_region ( 0.0, 0.0, max (last_canvas_unit, canvas_width), canvas_height);

	controls_layout.queue_resize();
}

void
Editor::controls_layout_size_request (Requisition* req)
{
	TreeModel::Children rows = route_display_model->children();
	TreeModel::Children::iterator i;
	double pos;

        for (pos = 0, i = rows.begin(); i != rows.end(); ++i) {
	        TimeAxisView *tv = (*i)[route_display_columns.tv];
		if (tv != 0) {
			pos += tv->effective_height;
			pos += track_spacing;
		}
	}

	RefPtr<Gdk::Screen> screen = get_screen();

	if (!screen) {
		screen = Gdk::Screen::get_default();
	}

	/* never let the width of the controls area shrink horizontally */

	edit_controls_vbox.check_resize();

	req->width = max (edit_controls_vbox.get_width(),  controls_layout.get_width());
	
	/* don't get too big. the fudge factors here are just guesses */
	
	req->width = min (req->width, screen->get_width() - 300);
	req->height = min ((gint) pos, (screen->get_height() - 400));

	/* this one is important: it determines how big the layout thinks it really is, as 
	   opposed to what it displays on the screen
	*/

	controls_layout.set_size (req->width, (gint) pos);
}

bool
Editor::track_canvas_map_handler (GdkEventAny* ev)
{
	track_canvas.get_window()->set_cursor (*current_canvas_cursor);
	return false;
}

bool
Editor::time_canvas_map_handler (GdkEventAny* ev)
{
	time_canvas.get_window()->set_cursor (*timebar_cursor);
	return false;
}

void  
Editor::track_canvas_drag_data_received (const RefPtr<Gdk::DragContext>& context,
					 int x, int y, 
					 const SelectionData& data,
					 guint info, guint time)
{
	if (data.get_target() == "regions") {
		drop_regions (context, x, y, data, info, time);
	} else {
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
	AudioTimeAxisView* tv;
	double cy;
	vector<ustring> paths;
	string spath;
	GdkEvent ev;
	nframes_t frame;

	if (convert_drop_to_paths (paths, context, x, y, data, info, time)) {
		goto out;
	}

	/* D-n-D coordinates are window-relative, so convert to "world" coordinates
	 */

	double wx;
	double wy;

	track_canvas.window_to_world (x, y, wx, wy);
	wx += horizontal_adjustment.get_value();
	wy += vertical_adjustment.get_value();
	
	ev.type = GDK_BUTTON_RELEASE;
	ev.button.x = wx;
	ev.button.y = wy;

	frame = event_frame (&ev, 0, &cy);

	snap_to (frame);

	if ((tvp = trackview_by_y_position (cy)) == 0) {

		/* drop onto canvas background: create new tracks */

		nframes_t pos = 0;
		do_embed (paths, false, ImportAsTrack, 0, pos, false);
		
	} else if ((tv = dynamic_cast<AudioTimeAxisView*>(tvp)) != 0) {

		/* check that its an audio track, not a bus */
		
		if (tv->get_diskstream()) {
			do_embed (paths, false, ImportToTrack, tv->audio_track(), frame, true);
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
		boost::shared_ptr<AudioRegion> ar;

		if ((ar = boost::dynamic_pointer_cast<AudioRegion>(r)) != 0) {
			insert_region_list_drag (ar, x, y);
		}
	}

	context->drag_finish (true, false, time);
}

void
Editor::maybe_autoscroll (GdkEvent* event)
{
	nframes_t rightmost_frame = leftmost_frame + current_page_frames();
	nframes_t frame = drag_info.current_pointer_frame;
	bool startit = false;

	static int last_autoscroll_direction = 0;

	if (frame > rightmost_frame) {

		if (rightmost_frame < max_frames) {
			autoscroll_direction = 1;
			startit = true;
		}

	} else if (frame < leftmost_frame) {

		if (leftmost_frame > 0) {
			autoscroll_direction = -1;
			startit = true;
		}

	} else {

		if (drag_info.last_pointer_frame > drag_info.current_pointer_frame) {
			autoscroll_direction = -1;
		} else {
			autoscroll_direction = 1;
		}
	}


	if ((autoscroll_direction != last_autoscroll_direction) || (leftmost_frame < frame < rightmost_frame)) {
		stop_canvas_autoscroll ();
	}
	
	if (startit && autoscroll_timeout_tag < 0) {
		start_canvas_autoscroll (autoscroll_direction);
	}

	last_autoscroll_direction = autoscroll_direction;
}

gint
Editor::_autoscroll_canvas (void *arg)
{
        return ((Editor *) arg)->autoscroll_canvas ();
}

bool
Editor::autoscroll_canvas ()
{
	nframes_t new_frame;
	nframes_t limit = max_frames - current_page_frames();
	GdkEventMotion ev;
	nframes_t target_frame;

	if (autoscroll_direction < 0) {
		if (leftmost_frame < autoscroll_distance) {
			new_frame = 0;
		} else {
			new_frame = leftmost_frame - autoscroll_distance;
		}
		target_frame = drag_info.current_pointer_frame - autoscroll_distance;
 	} else {
		if (leftmost_frame > limit - autoscroll_distance) {
			new_frame = limit;
		} else {
			new_frame = leftmost_frame + autoscroll_distance;
		}
		target_frame = drag_info.current_pointer_frame + autoscroll_distance;
	}

	/* now fake a motion event to get the object that is being dragged to move too */

	ev.type = GDK_MOTION_NOTIFY;
	ev.state &= Gdk::BUTTON1_MASK;
	ev.x = frame_to_unit (target_frame);
	ev.y = drag_info.current_pointer_y;
	motion_handler (drag_info.item, (GdkEvent*) &ev, drag_info.item_type, true);

	if (new_frame == 0 || new_frame == limit) {
		/* we are done */
		return false;
	}

	autoscroll_cnt++;

	if (autoscroll_cnt == 1) {

		/* connect the timeout so that we get called repeatedly */

		autoscroll_timeout_tag = g_idle_add ( _autoscroll_canvas, this);
		return false;

	} 

	if (new_frame != leftmost_frame) {
		reset_x_origin (new_frame);
	}

	if (autoscroll_cnt == 50) { /* 0.5 seconds */
		
		/* after about a while, speed up a bit by changing the timeout interval */

		autoscroll_distance = (nframes_t) floor (current_page_frames()/30.0f);
		
	} else if (autoscroll_cnt == 150) { /* 1.0 seconds */

		autoscroll_distance = (nframes_t) floor (current_page_frames()/20.0f);

	} else if (autoscroll_cnt == 300) { /* 1.5 seconds */

		/* after about another while, speed up by increasing the shift per callback */

		autoscroll_distance =  (nframes_t) floor (current_page_frames()/10.0f);

	} 

	return true;
}

void
Editor::start_canvas_autoscroll (int dir)
{
	if (!session || autoscroll_active) {
		return;
	}

	stop_canvas_autoscroll ();

	autoscroll_active = true;
	autoscroll_direction = dir;
	autoscroll_distance = (nframes_t) floor (current_page_frames()/50.0);
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

gint
Editor::left_track_canvas (GdkEventCrossing *ev)
{
	set_entered_track (0);
	set_entered_regionview (0);
	return FALSE;
}


void 
Editor::canvas_horizontally_scrolled ()
{
	/* this is the core function that controls horizontal scrolling of the canvas. it is called
	   whenever the horizontal_adjustment emits its "value_changed" signal. it typically executes in an
	   idle handler, which is important because tempo_map_changed() should issue redraws immediately
	   and not defer them to an idle handler.
	*/

  	leftmost_frame = (nframes_t) floor (horizontal_adjustment.get_value() * frames_per_unit);
	nframes_t rightmost_frame = leftmost_frame + current_page_frames ();
	
	if (rightmost_frame > last_canvas_frame) {
		last_canvas_frame = rightmost_frame;
		reset_scrolling_region ();
	}
	
	update_fixed_rulers ();

	tempo_map_changed (Change (0), !_dragging_hscrollbar);
}

