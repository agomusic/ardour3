// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _LIBGNOMECANVASMM_CANVAS_H
#define _LIBGNOMECANVASMM_CANVAS_H

#include <glibmm.h>

// -*- C++ -*-
/* $Id$ */

/* canvas.h
 * 
 * Copyright (C) 1998 EMC Capital Management Inc.
 * Developed by Havoc Pennington <hp@pobox.com>
 *
 * Copyright (C) 1999 The Gtk-- Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvasmm/affinetrans.h>
#include <gtkmm/layout.h>
#include <gdkmm/color.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GnomeCanvas GnomeCanvas;
typedef struct _GnomeCanvasClass GnomeCanvasClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gnome
{

namespace Canvas
{ class Canvas_Class; } // namespace Canvas

} // namespace Gnome
namespace Gnome
{

namespace Canvas
{

class Item;
class Group;

/** Canvas functions usually operate in either World coordinates
 * (units for the entire canvas), or Canvas coordinates (pixels starting 
 * at 0,0 in the top left).  There are functions to transform from 
 * one to the other.
 */

class Canvas : public Gtk::Layout
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef Canvas CppObjectType;
  typedef Canvas_Class CppClassType;
  typedef GnomeCanvas BaseObjectType;
  typedef GnomeCanvasClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~Canvas();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

protected:
  friend class Canvas_Class;
  static CppClassType canvas_class_;

  // noncopyable
  Canvas(const Canvas&);
  Canvas& operator=(const Canvas&);

protected:
  explicit Canvas(const Glib::ConstructParams& construct_params);
  explicit Canvas(GnomeCanvas* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GnomeCanvas*       gobj()       { return reinterpret_cast<GnomeCanvas*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GnomeCanvas* gobj() const { return reinterpret_cast<GnomeCanvas*>(gobject_); }


public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::
  virtual void on_draw_background(const Glib::RefPtr<Gdk::Drawable>& drawable, int x, int y, int width, int height);
  virtual void on_render_background(GnomeCanvasBuf* buf);


private:
;
public:
  Canvas();

  //Allow CanvasAA to access the canvas_class_ member.
  

  //: Get the root canvas item
  
  /** Queries the root group of a canvas.
   * @return The root group of the specified canvas.
   */
  Group* root() const;
           
  //: Limits of scroll region
  
  /** Sets the scrolling region of a canvas to the specified rectangle.  The canvas
   * will then be able to scroll only within this region.  The view of the canvas
   * is adjusted as appropriate to display as much of the new region as possible.
   * @param x1 Leftmost limit of the scrolling region.
   * @param y1 Upper limit of the scrolling region.
   * @param x2 Rightmost limit of the scrolling region.
   * @param y2 Lower limit of the scrolling region.
   */
  void set_scroll_region(double x1, double y1, double x2, double y2);

  //: Get limits of scroll region
  
  /** Queries the scrolling region of a canvas.
   * @param x1 Leftmost limit of the scrolling region (return value).
   * @param y1 Upper limit of the scrolling region (return value).
   * @param x2 Rightmost limit of the scrolling region (return value).
   * @param y2 Lower limit of the scrolling region (return value).
   */
  void get_scroll_region(double& x1, double& y1, double& x2, double& y2) const;

  
  /** When the scrolling region of the canvas is smaller than the canvas window,
   * e.g.\  the allocation of the canvas, it can be either centered on the window
   * or simply made to be on the upper-left corner on the window.  This function
   * lets you configure this property.
   * @param center_scroll_region Whether to center the scrolling region in the canvas
   * window when it is smaller than the canvas' allocation.
   */
  void set_center_scroll_region(bool center);

  
  /** Returns whether the canvas is set to center the scrolling region in the window
   * if the former is smaller than the canvas' allocation.
   * @return Whether the scroll region is being centered in the canvas window.
   */
  bool get_center_scroll_region() const;

  //: Set the pixels/world coordinates ratio
  //- With no arguments sets to default of 1.0.
  
  /** Sets the zooming factor of a canvas by specifying the number of pixels that
   * correspond to one canvas unit.
   * 
   * The anchor point for zooming, i.e. the point that stays fixed and all others
   * zoom inwards or outwards from it, depends on whether the canvas is set to
   * center the scrolling region or not.  You can control this using the
   * set_center_scroll_region() function.  If the canvas is set to
   * center the scroll region, then the center of the canvas window is used as the
   * anchor point for zooming.  Otherwise, the upper-left corner of the canvas
   * window is used as the anchor point.
   * @param n The number of pixels that correspond to one canvas unit.
   */
  void set_pixels_per_unit(double n = 1.0);

  //: Shift window.
  //- Makes a canvas scroll to the specified offsets, given in canvas pixel
  //- units.
  //- The canvas will adjust the view so that it is not outside the scrolling
  //- region.  This function is typically not used, as it is better to hook
  //- scrollbars to the canvas layout's scrolling adjusments.
  
  /** Makes a canvas scroll to the specified offsets, given in canvas pixel units.
   * The canvas will adjust the view so that it is not outside the scrolling
   * region.  This function is typically not used, as it is better to hook
   * scrollbars to the canvas layout's scrolling adjusments.
   * @param cx Horizontal scrolling offset in canvas pixel units.
   * @param cy Vertical scrolling offset in canvas pixel units.
   */
  void scroll_to(int x, int y);

  //: Scroll offsets in canvas pixel coordinates.
  
  /** Queries the scrolling offsets of a canvas.  The values are returned in canvas
   * pixel units.
   * @param cx Horizontal scrolling offset (return value).
   * @param cy Vertical scrolling offset (return value).
   */
  void get_scroll_offsets(int& cx, int& cy) const;

  //: Repaint immediately, don't wait for idle loop
  //- normally the canvas queues repainting and does it in an
  //- idle loop
  
  /** Forces an immediate update and redraw of a canvas.  If the canvas does not
   * have any pending update or redraw requests, then no action is taken.  This is
   * typically only used by applications that need explicit control of when the
   * display is updated, like games.  It is not needed by normal applications.
   */
  void update_now();

  //: Find an item at a location.
  //- Looks for the item that is under the specified position, which must be
  //- specified in world coordinates.  Arguments are in world coordinates.
  //- Returns 0 if no item is at that
  //- location.
  
  /** Looks for the item that is under the specified position, which must be
   * specified in world coordinates.
   * @param x X position in world coordinates.
   * @param y Y position in world coordinates.
   * @return The sought item, or <tt>0</tt> if no item is at the specified
   * coordinates.
   */
  Item* get_item_at(double x, double y) const;


  //: Repaint small area (internal)
  //- Used only by item implementations. Request an eventual redraw
  //- of the region, which includes x1,y1 but not x2,y2
  
  /** Convenience function that informs a canvas that the specified rectangle needs
   * to be repainted.  This function converts the rectangle to a microtile array
   * and feeds it to request_redraw_uta().  The rectangle includes
   *  @a x1  and @a y1 , but not @a x2  and @a y2 .  To be used only by item implementations.
   * @param x1 Leftmost coordinate of the rectangle to be redrawn.
   * @param y1 Upper coordinate of the rectangle to be redrawn.
   * @param x2 Rightmost coordinate of the rectangle to be redrawn, plus 1.
   * @param y2 Lower coordinate of the rectangle to be redrawn, plus 1.
   */
  void request_redraw(int x1, int y1, int x2, int y2);
  //TODO: Investigate ArtUta.
  
  /** Informs a canvas that the specified area, given as a microtile array, needs
   * to be repainted.  To be used only by item implementations.
   * @param uta Microtile array that specifies the area to be redrawn.  It will
   * be freed by this function, so the argument you pass will be invalid
   * after you call this function.
   */
  void request_redraw(ArtUta* uta);

  Art::AffineTrans w2c_affine() const;
  

  //: Convert from World to canvas coordinates (units for the entire canvas)
  //: to Canvas coordinates (pixels starting at 0,0 in the top left
  //: of the visible area). The relationship depends on the current
  //: scroll position and the pixels_per_unit ratio (zoom factor)
  
  /** Converts world coordinates into canvas pixel coordinates.
   * @param wx World X coordinate.
   * @param wy World Y coordinate.
   * @param cx X pixel coordinate (return value).
   * @param cy Y pixel coordinate (return value).
   */
  void w2c(double wx, double wy, int& cx, int& cy) const;
  
  /** Converts world coordinates into canvas pixel coordinates.  This version
   * @param wx World X coordinate.
   * @param wy World Y coordinate.
   * @param cx X pixel coordinate (return value).
   * @param cy Y pixel coordinate (return value).
   * @return Coordinates in floating point coordinates, for greater precision.
   */
  void w2c(double wx, double wy, double& cx, double& cy) const;

  //: From Canvas to World
  
  /** Converts canvas pixel coordinates to world coordinates.
   * @param cx Canvas pixel X coordinate.
   * @param cy Canvas pixel Y coordinate.
   * @param wx X world coordinate (return value).
   * @param wy Y world coordinate (return value).
   */
  void c2w(int cx, int cy, double& wx, double& wy) const;

  //: Convert from Window coordinates to world coordinates.
  //- Window coordinates are based of the widget's GdkWindow.
  //- This is fairly low-level and not generally useful.
  
  /** Converts window-relative coordinates into world coordinates.  You can use
   * this when you need to convert mouse coordinates into world coordinates, for
   * example.
   * @param winx Window-relative X coordinate.
   * @param winy Window-relative Y coordinate.
   * @param worldx X world coordinate (return value).
   * @param worldy Y world coordinate (return value).
   */
  void window_to_world (double winx,double winy, double& worldx,double& worldy) const;

  //: Convert from world coordinates to Window coordinates.
  //- Window coordinates are based of the widget's GdkWindow.
  //- This is fairly low-level and not generally useful.
  
  /** Converts world coordinates into window-relative coordinates.
   * @param worldx World X coordinate.
   * @param worldy World Y coordinate.
   * @param winx X window-relative coordinate.
   * @param winy Y window-relative coordinate.
   */
  void world_to_window (double worldx, double worldy, double& winx, double& winy) const;

  //: Parse color spec string and allocate it into the GdkColor.
  bool get_color(const Glib::ustring& spec, Gdk::Color& color) const;
  

/* Allocates a color from the RGB value passed into this function. */
  
  /** Allocates a color from the RGBA value passed into this function.  The alpha
   * opacity value is discarded, since normal X colors do not support it.
   * @param rgba RGBA color specification.
   * @return Allocated pixel value corresponding to the specified color.
   */
  gulong get_color_pixel(guint rgba) const;
  
  /** Sets the stipple origin of the specified GC as is appropriate for the canvas,
   * so that it will be aligned with other stipple patterns used by canvas items.
   * This is typically only needed by item implementations.
   * @param gc GC on which to set the stipple origin.
   */
  void set_stipple_origin(const Glib::RefPtr<Gdk::GC>& gc);
  
  /** Controls dithered rendering for antialiased canvases. The value of
   * dither should be Gdk::RGB_DITHER_NONE, Gdk::RGB_DITHER_NORMAL, or
   * Gdk::RGB_DITHER_MAX. The default canvas setting is
   * Gdk::RGB_DITHER_NORMAL.
   * @param dither Type of dithering used to render an antialiased canvas.
   */
  void set_dither(Gdk::RgbDither dither);
  
  /** Returns the type of dithering used to render an antialiased canvas.
   * @return The dither setting.
   */
  Gdk::RgbDither get_dither() const;


  //TODO: Look at ArtSVP.
  
  /** Sets the svp to the new value, requesting repaint on what's changed. This
   * function takes responsibility for freeing new_svp.
   * @param p_svp A pointer to the existing svp.
   * @param new_svp The new svp.
   */
  void update_svp(ArtSVP** p_svp, ArtSVP* new_svp);
  
  /** Sets the svp to the new value, clipping if necessary, and requesting repaint
   * on what's changed. This function takes responsibility for freeing new_svp.
   * @param p_svp A pointer to the existing svp.
   * @param new_svp The new svp.
   * @param clip_svp A clip path, if non-null.
   */
  void update_svp_clip(ArtSVP** p_svp, ArtSVP* new_svp, ArtSVP* clip_svp);

  // The following are simply accessed via the struct in C,
  //  but Federico reports that they are meant to be used.
  //: Get the pixels per unit.
  double get_pixels_per_unit() const;

  //: Draw the background for the area given.
  //- This method is only used for non-antialiased canvases.
  

  Glib::SignalProxy5< void,const Glib::RefPtr<Gdk::Drawable>&,int,int,int,int > signal_draw_background();

  // Render the background for the buffer given. 
  //- The buf data structure contains both a pointer to a packed 24-bit
  //- RGB array, and the coordinates.
  //- This method is only used for antialiased canvases.
  

  Glib::SignalProxy1< void,GnomeCanvasBuf* > signal_render_background();

  //: Private Virtual methods for groping the canvas inside bonobo.
    virtual void request_update_vfunc();

  // Whether the canvas is in antialiased mode or not.
  /** 
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_aa() ;

/** 
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_aa() const;


};

//: Antialiased Canvas.
//- Constructor takes care of push/pop actions of the colormap.
class CanvasAA : public Canvas
{
  public:
    CanvasAA();
    virtual ~CanvasAA();
};

} /* namespace Canvas */
} /* namespace Gnome */


namespace Glib
{
  /** @relates Gnome::Canvas::Canvas
   * @param object The C instance
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   */
  Gnome::Canvas::Canvas* wrap(GnomeCanvas* object, bool take_copy = false);
}
#endif /* _LIBGNOMECANVASMM_CANVAS_H */

