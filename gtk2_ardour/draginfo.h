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

#ifndef __gtk2_ardour_drag_info_h_
#define __gtk2_ardour_drag_info_h_

#include <gdk/gdk.h>
#include <stdint.h>

#include "canvas.h"
#include "editor_items.h"

#include <ardour/types.h>

namespace ARDOUR {
	class Location;
}

class Editor;
class TimeAxisView;

struct DragInfo {
    ArdourCanvas::Item* item;
    ItemType            item_type;
    void* data;
    nframes_t last_frame_position;
    int64_t pointer_frame_offset;
    nframes_t grab_frame;
    nframes_t last_pointer_frame;
    nframes_t current_pointer_frame;
    double grab_x, grab_y;
    double cumulative_x_drag;
    double cumulative_y_drag;
    double current_pointer_x;
    double current_pointer_y;
    void (Editor::*motion_callback)(ArdourCanvas::Item*, GdkEvent*);
    void (Editor::*finished_callback)(ArdourCanvas::Item*, GdkEvent*);
    TimeAxisView* last_trackview;
    bool x_constrained;
    bool y_constrained;
    bool copy;
    bool was_rolling;
    bool first_move;
    bool move_threshold_passed;
    bool want_move_threshold;
    bool brushing;
    ARDOUR::Location* copied_location;
};

struct LineDragInfo {
    uint32_t before;
    uint32_t after;
};

#endif /* __gtk2_ardour_drag_info_h_ */

