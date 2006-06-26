/*
    Copyright (C) 2003 Paul Davis 

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

#include <pbd/error.h>

#include <ardour/types.h>
#include <ardour/ardour.h>

#include <gtkmm2ext/utils.h>

#include "public_editor.h"
#include "time_axis_view_item.h"
#include "time_axis_view.h"
#include "simplerect.h"
#include "utils.h"
#include "canvas_impl.h"
#include "rgb_macros.h"

#include "i18n.h"

using namespace std;
using namespace Editing;
using namespace Glib;
using namespace PBD;

//------------------------------------------------------------------------------
/** Initialize const static memeber data */

Pango::FontDescription TimeAxisViewItem::NAME_FONT;
bool TimeAxisViewItem::have_name_font = false;
const double TimeAxisViewItem::NAME_X_OFFSET = 15.0;
const double TimeAxisViewItem::GRAB_HANDLE_LENGTH = 6 ;

double TimeAxisViewItem::NAME_Y_OFFSET;
double TimeAxisViewItem::NAME_HIGHLIGHT_SIZE;
double TimeAxisViewItem::NAME_HIGHLIGHT_THRESH;


//---------------------------------------------------------------------------------------//
// Constructor / Desctructor

/**
 * Constructs a new TimeAxisViewItem.
 *
 * @param it_name the unique name/Id of this item
 * @param parant the parent canvas group
 * @param tv the TimeAxisView we are going to be added to
 * @param spu samples per unit
 * @param base_color
 * @param start the start point of this item
 * @param duration the duration of this item
 */
TimeAxisViewItem::TimeAxisViewItem(const string & it_name, ArdourCanvas::Group& parent, TimeAxisView& tv, double spu, Gdk::Color& base_color, 
				   jack_nframes_t start, jack_nframes_t duration,
				   Visibility vis)
	: trackview (tv)
{
	if (!have_name_font) {

		/* first constructed item sets up font info */

		NAME_FONT = get_font_for_style (N_("TimeAxisViewItemName"));
		
		Gtk::Window win;
		Gtk::Label foo;
		win.add (foo);

		Glib::RefPtr<Pango::Layout> layout = foo.create_pango_layout (X_("Hg")); /* ascender + descender */
		int width;
		int height;

		layout->set_font_description (NAME_FONT);
		Gtkmm2ext::get_ink_pixel_size (layout, width, height);

		NAME_Y_OFFSET = height + 4;
		NAME_HIGHLIGHT_SIZE = height + 6;
		NAME_HIGHLIGHT_THRESH = NAME_HIGHLIGHT_SIZE * 2;

		have_name_font = true;
	}

	item_name = it_name ;
	samples_per_unit = spu ;
	should_show_selection = true;
	frame_position = start ;
	item_duration = duration ;
	name_connected = false;
	fill_opacity = 50;
	position_locked = false ;
	max_item_duration = ARDOUR::max_frames;
	min_item_duration = 0 ;
	show_vestigial = true;
	visibility = vis;

	if (duration == 0) {
		warning << "Time Axis Item Duration == 0" << endl ;
	}

	group = new ArdourCanvas::Group (parent);

	vestigial_frame = new ArdourCanvas::SimpleRect (*group);
	vestigial_frame->property_x1() = (double) 0.0;
	vestigial_frame->property_y1() = (double) 1.0;
	vestigial_frame->property_x2() = 2.0;
	vestigial_frame->property_y2() = (double) trackview.height;
	vestigial_frame->property_outline_color_rgba() = color_map[cVestigialFrameOutline];
	vestigial_frame->property_fill_color_rgba() = color_map[cVestigialFrameFill];
	vestigial_frame->hide ();

	if (visibility & ShowFrame) {
		frame = new ArdourCanvas::SimpleRect (*group);
		frame->property_x1() = (double) 0.0;
		frame->property_y1() = (double) 1.0;
		frame->property_x2() = (double) trackview.editor.frame_to_pixel(duration);
		frame->property_y2() = (double) trackview.height;
		frame->property_outline_color_rgba() = color_map[cTimeAxisFrameOutline];
		frame->property_fill_color_rgba() = color_map[cTimeAxisFrameFill];

		/* by default draw all 4 edges */

		uint32_t outline_what = 0x1|0x2|0x4|0x8;

		if (visibility & HideFrameLR) {
			outline_what &= ~(0x1 | 0x2);
		}

		if (visibility & HideFrameTB) {
			outline_what &= ~(0x4 | 0x8);
		}

		frame->property_outline_what() = outline_what;
		    
	} else {
		frame = 0;
	}

	if (visibility & ShowNameHighlight) {
		name_highlight = new ArdourCanvas::SimpleRect (*group);
		if (visibility & FullWidthNameHighlight) {
			name_highlight->property_x1() = (double) 0.0;
			name_highlight->property_x2() = (double) (trackview.editor.frame_to_pixel(item_duration));
		} else {
			name_highlight->property_x1() = (double) 1.0;
			name_highlight->property_x2() = (double) (trackview.editor.frame_to_pixel(item_duration)) - 1;
		}
		name_highlight->property_y1() = (double) (trackview.height - TimeAxisViewItem::NAME_HIGHLIGHT_SIZE);
		name_highlight->property_y2() = (double) (trackview.height - 1);
		name_highlight->property_outline_color_rgba() = color_map[cNameHighlightFill];
		name_highlight->property_fill_color_rgba() = color_map[cNameHighlightOutline];

		name_highlight->set_data ("timeaxisviewitem", this);

	} else {
		name_highlight = 0;
	}

	if (visibility & ShowNameText) {
		name_text = new ArdourCanvas::Text (*group);
		name_text->property_x() = (double) TimeAxisViewItem::NAME_X_OFFSET;
		/* trackview.height is the bottom of the trackview. subtract 1 to get back to the bottom of the highlight,
		   then NAME_Y_OFFSET to position the text in the vertical center of the highlight
		*/
		name_text->property_y() = (double) trackview.height - 1.0 - TimeAxisViewItem::NAME_Y_OFFSET;
		name_text->property_font_desc() = NAME_FONT;
		name_text->property_anchor() = Gtk::ANCHOR_NW;

		name_text->set_data ("timeaxisviewitem", this);
		
	} else {
		name_text = 0;
	}

	/* create our grab handles used for trimming/duration etc */

	if (visibility & ShowHandles) {
		frame_handle_start = new ArdourCanvas::SimpleRect (*group);
		frame_handle_start->property_x1() = (double) 0.0;
		frame_handle_start->property_x2() = (double) TimeAxisViewItem::GRAB_HANDLE_LENGTH;
		frame_handle_start->property_y1() = (double) 1.0;
		frame_handle_start->property_y2() = (double) TimeAxisViewItem::GRAB_HANDLE_LENGTH+1;
		frame_handle_start->property_outline_color_rgba() = color_map[cFrameHandleStartOutline];
		frame_handle_start->property_fill_color_rgba() = color_map[cFrameHandleStartFill];
		
		frame_handle_end = new ArdourCanvas::SimpleRect (*group);
		frame_handle_end->property_x1() = (double) (trackview.editor.frame_to_pixel(get_duration())) - (TimeAxisViewItem::GRAB_HANDLE_LENGTH);
		frame_handle_end->property_x2() = (double) trackview.editor.frame_to_pixel(get_duration());
		frame_handle_end->property_y1() = (double) 1;
		frame_handle_end->property_y2() = (double) TimeAxisViewItem::GRAB_HANDLE_LENGTH + 1;
		frame_handle_end->property_outline_color_rgba() = color_map[cFrameHandleEndOutline];
		frame_handle_end->property_fill_color_rgba() = color_map[cFrameHandleEndFill];

	} else {
		frame_handle_start = 0;
		frame_handle_end = 0;
	}

	set_color (base_color) ;

	set_duration (item_duration, this) ;
	set_position (start, this) ;
}


/**
 * Destructor
 */
TimeAxisViewItem::~TimeAxisViewItem()
{
	delete group;
}


//---------------------------------------------------------------------------------------//
// Position and duration Accessors/Mutators

/**
 * Set the position of this item upon the timeline to the specified value
 *
 * @param pos the new position
 * @param src the identity of the object that initiated the change
 * @return true if the position change was a success, false otherwise
 */
bool
TimeAxisViewItem::set_position(jack_nframes_t pos, void* src, double* delta)
{
	if (position_locked) {
		return false;
	}

	frame_position = pos;
	
	/*  This sucks. The GnomeCanvas version I am using
	    doesn't correctly implement gnome_canvas_group_set_arg(),
	    so that simply setting the "x" arg of the group
	    fails to move the group. Instead, we have to
	    use gnome_canvas_item_move(), which does the right
	    thing. I see that in GNOME CVS, the current (Sept 2001)
	    version of GNOME Canvas rectifies this issue cleanly.
	*/
	
	double old_unit_pos ;
	double new_unit_pos = pos / samples_per_unit ;

	old_unit_pos = group->property_x();

	if (new_unit_pos != old_unit_pos) {
		group->move (new_unit_pos - old_unit_pos, 0.0);
	}
	
	if (delta) {
		(*delta) = new_unit_pos - old_unit_pos;
	}
	
	PositionChanged (frame_position, src) ; /* EMIT_SIGNAL */

	return true;
}

/**
 * Return the position of this item upon the timeline
 *
 * @return the position of this item
 */
jack_nframes_t
TimeAxisViewItem::get_position() const
{
	return frame_position;
}

/**
 * Sets the duration of this item
 *
 * @param dur the new duration of this item
 * @param src the identity of the object that initiated the change
 * @return true if the duration change was succesful, false otherwise
 */
bool
TimeAxisViewItem::set_duration (jack_nframes_t dur, void* src)
{
	if ((dur > max_item_duration) || (dur < min_item_duration)) {
		warning << string_compose (_("new duration %1 frames is out of bounds for %2"), get_item_name(), dur)
			<< endmsg;
		return false;
	}

	if (dur == 0) {
		group->hide();
	}

	item_duration = dur;
	
	double pixel_width = trackview.editor.frame_to_pixel (dur);

	reset_width_dependent_items (pixel_width);
	
	DurationChanged (dur, src) ; /* EMIT_SIGNAL */
	return true;
}

/**
 * Returns the duration of this item
 *
 */
jack_nframes_t
TimeAxisViewItem::get_duration() const
{
	return (item_duration);
}

/**
 * Sets the maximum duration that this item make have.
 *
 * @param dur the new maximum duration
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_max_duration(jack_nframes_t dur, void* src)
{
	max_item_duration = dur ;
	MaxDurationChanged(max_item_duration, src) ; /* EMIT_SIGNAL */
}
		
/**
 * Returns the maxmimum duration that this item may be set to
 *
 * @return the maximum duration that this item may be set to
 */
jack_nframes_t
TimeAxisViewItem::get_max_duration() const
{
	return (max_item_duration) ;
}

/**
 * Sets the minimu duration that this item may be set to
 *
 * @param the minimum duration that this item may be set to
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_min_duration(jack_nframes_t dur, void* src)
{
	min_item_duration = dur ;
	MinDurationChanged(max_item_duration, src) ; /* EMIT_SIGNAL */
}
		
/**
 * Returns the minimum duration that this item mey be set to
 *
 * @return the nimum duration that this item mey be set to
 */
jack_nframes_t
TimeAxisViewItem::get_min_duration() const
{
	return(min_item_duration) ;
}

/**
 * Sets whether the position of this Item is locked to its current position
 * Locked items cannot be moved until the item is unlocked again.
 *
 * @param yn set to true to lock this item to its current position
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_position_locked(bool yn, void* src)
{
	position_locked = yn ;
	set_trim_handle_colors() ;
	PositionLockChanged (position_locked, src); /* EMIT_SIGNAL */
}

/**
 * Returns whether this item is locked to its current position
 *
 * @return true if this item is locked to its current posotion
 *         false otherwise
 */
bool
TimeAxisViewItem::get_position_locked() const
{
	return (position_locked);
}

/**
 * Sets whether the Maximum Duration constraint is active and should be enforced
 *
 * @param active set true to enforce the max duration constraint
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_max_duration_active(bool active, void* src)
{
	max_duration_active = active ;
}
		
/**
 * Returns whether the Maximum Duration constraint is active and should be enforced
 *
 * @return true if the maximum duration constraint is active, false otherwise
 */
bool
TimeAxisViewItem::get_max_duration_active() const
{
	return(max_duration_active) ;
}
		
/**
 * Sets whether the Minimum Duration constraint is active and should be enforced
 *
 * @param active set true to enforce the min duration constraint
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_min_duration_active(bool active, void* src)
{
	min_duration_active = active ;
}
		
/**
 * Returns whether the Maximum Duration constraint is active and should be enforced
 *
 * @return true if the maximum duration constraint is active, false otherwise
 */
bool
TimeAxisViewItem::get_min_duration_active() const
{
	return(min_duration_active) ;
}

//---------------------------------------------------------------------------------------//
// Name/Id Accessors/Mutators

/**
 * Set the name/Id of this item.
 *
 * @param new_name the new name of this item
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_item_name(std::string new_name, void* src)
{
	if (new_name != item_name) {
		std::string temp_name = item_name ;
		item_name = new_name ;
		NameChanged (item_name, temp_name, src) ; /* EMIT_SIGNAL */
	}
}

/**
 * Returns the name/id of this item
 *
 * @return the name/id of this item
 */
std::string
TimeAxisViewItem::get_item_name() const
{
	return(item_name) ;
}

//---------------------------------------------------------------------------------------//
// Selection Methods

/**
 * Set to true to indicate that this item is currently selected
 *
 * @param yn true if this item is currently selected
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::set_selected(bool yn)
{
	if (_selected != yn) {
		Selectable::set_selected (yn);
		set_frame_color ();
	}
}

void 
TimeAxisViewItem::set_should_show_selection (bool yn)
{
	if (should_show_selection != yn) {
		should_show_selection = yn;
		set_frame_color ();
	}
}

//---------------------------------------------------------------------------------------//
// Parent Componenet Methods

/**
 * Returns the TimeAxisView that this item is upon
 *
 * @return the timeAxisView that this item is placed upon
 */
TimeAxisView&
TimeAxisViewItem::get_time_axis_view()
{
	return trackview;
}		
//---------------------------------------------------------------------------------------//
// ui methods & data

/**
 * Sets the displayed item text
 * This item is the visual text name displayed on the canvas item, this can be different to the name of the item
 *
 * @param new_name the new name text to display
 */
void
TimeAxisViewItem::set_name_text(std::string new_name)
{
	if (name_text) {
		name_text->property_text() = new_name.c_str();
	}
}

/**
 * Set the height of this item
 *
 * @param h the new height
 */		
void
TimeAxisViewItem::set_height(double height)
{
	if (name_highlight) {
		if (height < NAME_HIGHLIGHT_THRESH) {
			name_highlight->hide();
			if (name_text) {
				name_text->hide();
			}
		} else {
			name_highlight->show();
			if (name_text) {
				name_text->show();
			}
		}

		if (height > NAME_HIGHLIGHT_SIZE) {
			name_highlight->property_y1() = (double) height+1 - NAME_HIGHLIGHT_SIZE;
			name_highlight->property_y2() = (double) height;
		}
		else {
			/* it gets hidden now anyway */
			name_highlight->property_y1() = (double) 1.0;
			name_highlight->property_y2() = (double) height;
		}
	}

	if (name_text) {
		name_text->property_y() = height+1 - NAME_Y_OFFSET;
		if (height < NAME_HIGHLIGHT_THRESH) {
			name_text->property_fill_color_rgba() =  fill_color;
		}
		else {
			name_text->property_fill_color_rgba() = label_color;
		}
	}

	if (frame) {
		frame->property_y2() = height+1;
	}

	vestigial_frame->property_y2() = height+1;
}

/**
 * 
 */
void
TimeAxisViewItem::set_color(Gdk::Color& base_color)
{
	compute_colors (base_color);
	set_colors ();
}

/**
 * 
 */
ArdourCanvas::Item*
TimeAxisViewItem::get_canvas_frame()
{
	return(frame) ;
}

/**
 * 
 */
ArdourCanvas::Item*
TimeAxisViewItem::get_canvas_group()
{
	return (group) ;
}

/**
 * 
 */
ArdourCanvas::Item*
TimeAxisViewItem::get_name_highlight()
{
	return (name_highlight) ;
}

/**
 * 
 */
ArdourCanvas::Text*
TimeAxisViewItem::get_name_text()
{
	return (name_text) ;
}

/**
 * Calculates some contrasting color for displaying various parts of this item, based upon the base color
 *
 * @param color the base color of the item
 */
void
TimeAxisViewItem::compute_colors(Gdk::Color& base_color)
{
	unsigned char radius ;
	char minor_shift ;
	
	unsigned char r,g,b ;

	/* FILL: this is simple */
	r = base_color.get_red()/256 ;
	g = base_color.get_green()/256 ;
	b = base_color.get_blue()/256 ;
	fill_color = RGBA_TO_UINT(r,g,b,255) ;

	/*  for minor colors:
		if the overall saturation is strong, make the minor colors light.
		if its weak, make them dark.
  
   		we do this by moving an equal distance to the other side of the
		central circle in the color wheel from where we started.
	*/

	radius = (unsigned char) rint (floor (sqrt (static_cast<double>(r*r + g*g + b+b))/3.0f)) ;
	minor_shift = 125 - radius ;

	/* LABEL: rotate around color wheel by 120 degrees anti-clockwise */

	r = base_color.get_red()/256;
	g = base_color.get_green()/256;
	b = base_color.get_blue()/256;
  
	if (r > b)
	{
		if (r > g)
		{
			/* red sector => green */
			swap (r,g);
		}
		else
		{
			/* green sector => blue */
			swap (g,b);
		} 
	}
	else
	{
		if (b > g)
		{
			/* blue sector => red */
			swap (b,r);
		}
		else
		{
			/* green sector => blue */
			swap (g,b);
		}
	}

	r += minor_shift;
	b += minor_shift;
	g += minor_shift;
  
	label_color = RGBA_TO_UINT(r,g,b,255);
	r = (base_color.get_red()/256)   + 127 ;
	g = (base_color.get_green()/256) + 127 ;
	b = (base_color.get_blue()/256)  + 127 ;
  
	label_color = RGBA_TO_UINT(r,g,b,255);

	/* XXX can we do better than this ? */
	/* We're trying ;) */
	/* NUKECOLORS */
	
	//frame_color_r = 192;
	//frame_color_g = 192;
	//frame_color_b = 194;
	
	//selected_frame_color_r = 182;
	//selected_frame_color_g = 145;
	//selected_frame_color_b = 168;
	
	//handle_color_r = 25 ;
	//handle_color_g = 0 ;
	//handle_color_b = 255 ;
	//lock_handle_color_r = 235 ;
	//lock_handle_color_g = 16;
	//lock_handle_color_b = 16;
}

/**
 * Convenience method to set the various canvas item colors
 */
void
TimeAxisViewItem::set_colors()
{
	set_frame_color() ;
	if (name_text) {
		double height = NAME_HIGHLIGHT_THRESH;

		if (frame) {
			height = frame->property_y2();
		}

		if (height < NAME_HIGHLIGHT_THRESH) {
			name_text->property_fill_color_rgba() =  fill_color;
		}
		else {
			name_text->property_fill_color_rgba() = label_color;
		}
	}

	if (name_highlight) {
		name_highlight->property_fill_color_rgba() = fill_color;
		name_highlight->property_outline_color_rgba() = fill_color;
	}
	set_trim_handle_colors() ;
}

/**
 * Sets the frame color depending on whether this item is selected
 */
void
TimeAxisViewItem::set_frame_color()
{
	if (frame) {
		uint32_t r,g,b,a;
		
		if (_selected && should_show_selection) {
			UINT_TO_RGBA(color_map[cSelectedFrameBase], &r, &g, &b, &a);
			frame->property_fill_color_rgba() = RGBA_TO_UINT(r, g, b, fill_opacity);
		} else {
			UINT_TO_RGBA(color_map[cFrameBase], &r, &g, &b, &a);
			frame->property_fill_color_rgba() = RGBA_TO_UINT(r, g, b, fill_opacity);
		}
	}
}

/**
 * Sets the colors of the start and end trim handle depending on object state
 *
 */
void
TimeAxisViewItem::set_trim_handle_colors()
{
	if (frame_handle_start) {
		if (position_locked) {
			frame_handle_start->property_fill_color_rgba() = color_map[cTrimHandleLockedStart];
			frame_handle_end->property_fill_color_rgba() = color_map[cTrimHandleLockedEnd];
		} else {
			frame_handle_start->property_fill_color_rgba() = color_map[cTrimHandleStart];
			frame_handle_end->property_fill_color_rgba() = color_map[cTrimHandleEnd];
		}
	}
}

double
TimeAxisViewItem::get_samples_per_unit()
{
	return(samples_per_unit) ;
}

void
TimeAxisViewItem::set_samples_per_unit (double spu)
{
	samples_per_unit = spu ;
	set_position (this->get_position(), this);
	reset_width_dependent_items ((double)get_duration() / samples_per_unit);
}

void
TimeAxisViewItem::reset_width_dependent_items (double pixel_width)
{
	if (pixel_width < GRAB_HANDLE_LENGTH * 2) {

		if (frame_handle_start) {
			frame_handle_start->hide();
			frame_handle_end->hide();
		}

	} if (pixel_width < 2.0) {

		if (show_vestigial) {
			vestigial_frame->show();
		}

		if (name_highlight) {
			name_highlight->hide();
			if (name_text) {
				name_text->hide();
			}
		}

		if (frame) {
			frame->hide();
		}

		if (frame_handle_start) {
			frame_handle_start->hide();
			frame_handle_end->hide();
		}
		
	} else {
		vestigial_frame->hide();

		if (name_highlight) {

			double height = name_highlight->property_y2 ();

			if (height < NAME_HIGHLIGHT_THRESH) {
				name_highlight->hide();
				if (name_text) {
					name_text->hide();
				}
			} else {
				name_highlight->show();
				if (name_text) {
					name_text->show();
					reset_name_width (pixel_width);
				}
			}

			if (visibility & FullWidthNameHighlight) {
				name_highlight->property_x2() = pixel_width;
			} else {
				name_highlight->property_x2() = pixel_width - 1.0;
			}

		}

		if (frame) {
			frame->show();
			frame->property_x2() = pixel_width;
		}

		if (frame_handle_start) {
			if (pixel_width < (2*TimeAxisViewItem::GRAB_HANDLE_LENGTH)) {
				frame_handle_start->hide();
				frame_handle_end->hide();
			}
			frame_handle_start->show();
			frame_handle_end->property_x1() = pixel_width - (TimeAxisViewItem::GRAB_HANDLE_LENGTH);
			frame_handle_end->show();
			frame_handle_end->property_x2() = pixel_width;
		}
	}
}

void
TimeAxisViewItem::reset_name_width (double pixel_width)
{
	if (name_text == 0) {
		return;
	}
                       
	int width;
	
	ustring ustr = fit_to_pixels (item_name, (int) floor (pixel_width - NAME_X_OFFSET), NAME_FONT, width);

	if (ustr.empty()) {
		
		name_text->hide ();
		
	} else {
		
		/* don't use name for event handling if it leaves no room
		   for trimming to work.
		*/
		
		if (pixel_width - width < (NAME_X_OFFSET * 2.0)) {
			if (name_connected) {
				name_connected = false;
			}
		} else {
			if (!name_connected) {
				name_connected = true;
			}
		}
		
		name_text->property_text() = ustr;
		name_text->show();
	}
}


//---------------------------------------------------------------------------------------//
// Handle time axis removal

/**
 * Handles the Removal of this time axis item
 * This _needs_ to be called to alert others of the removal properly, ie where the source
 * of the removal came from.
 *
 * XXX Although im not too happy about this method of doing things, I cant think of a cleaner method
 *     just now to capture the source of the removal
 *
 * @param src the identity of the object that initiated the change
 */
void
TimeAxisViewItem::remove_this_item(void* src)
{
	/*
	   defer to idle loop, otherwise we'll delete this object
	   while we're still inside this function ...
	*/
        Glib::signal_idle().connect(bind (sigc::ptr_fun (&TimeAxisViewItem::idle_remove_this_item), this, src));
}

/**
 * Callback used to remove this time axis item during the gtk idle loop
 * This is used to avoid deleting the obejct while inside the remove_this_item
 * method
 *
 * @param item the TimeAxisViewItem to remove
 * @param src the identity of the object that initiated the change
 */
gint
TimeAxisViewItem::idle_remove_this_item(TimeAxisViewItem* item, void* src)
{
	item->ItemRemoved (item->get_item_name(), src) ; /* EMIT_SIGNAL */
	delete item;
	item = 0;
	return false;
}
