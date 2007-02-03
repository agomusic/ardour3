// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_ICONSET_H
#define _GTKMM_ICONSET_H


#include <glibmm.h>

/* $Id$ */

/* iconset.h
 *
 * Copyright(C) 1998-2002 The gtkmm Development Team
 *
 * This library is free software, ) you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, ) either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, ) without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library, ) if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//#include <gtkmm/style.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/iconsource.h>
//#include <gtkmm/widget.h>
#include <gtkmm/stockid.h>
#include <glibmm/arrayhandle.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _GtkIconSet GtkIconSet; }
#endif

namespace Gtk
{

class Style;
class Widget;

//TODO_API: Is _CLASS_BOXEDTYPE the appropriate thing to use here.
//This seems to be reference-counted, not copied.

class IconSet
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef IconSet CppObjectType;
  typedef GtkIconSet BaseObjectType;

  static GType get_type() G_GNUC_CONST;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  IconSet();

  explicit IconSet(GtkIconSet* gobject, bool make_a_copy = true);

  IconSet(const IconSet& other);
  IconSet& operator=(const IconSet& other);

  ~IconSet();

  void swap(IconSet& other);

  ///Provides access to the underlying C instance.
  GtkIconSet*       gobj()       { return gobject_; }

  ///Provides access to the underlying C instance.
  const GtkIconSet* gobj() const { return gobject_; }

  ///Provides access to the underlying C instance. The caller is responsible for freeing it. Use when directly setting fields in structs.
  GtkIconSet* gobj_copy() const;

protected:
  GtkIconSet* gobject_;

private:

  
public:
  explicit IconSet(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf);

  
  /** Copies @a icon_set  by value.
   * @return A new Gtk::IconSet identical to the first.
   */
  IconSet copy() const;

  //Note that we use Gtk::StateType here instead of StateType, because there is an Atk::StateType too, and doxygen gets confused.
  
  /** Renders an icon using Gtk::Style::render_icon(). In most cases,
   * Gtk::Widget::render_icon() is better, since it automatically provides
   * most of the arguments from the current widget settings.  This
   * function never returns <tt>0</tt>; if the icon can't be rendered
   * (perhaps because an image file fails to load), a default "missing
   * image" icon will be returned instead.
   * @param style A Gtk::Style associated with @a widget , or <tt>0</tt>.
   * @param direction Text direction.
   * @param state Widget state.
   * @param size Icon size. A size of (GtkIconSize)-1
   * means render at the size of the source and don't scale.
   * @param widget Widget that will display the icon, or <tt>0</tt>.
   * The only use that is typically made of this
   * is to determine the appropriate Gdk::Screen.
   * @param detail Detail to pass to the theme engine, or <tt>0</tt>.
   * Note that passing a detail of anything but <tt>0</tt>
   * will disable caching.
   * @return A Gdk::Pixbuf to be displayed.
   */
  Glib::RefPtr<Gdk::Pixbuf> render_icon(const Glib::RefPtr<Style>& style, TextDirection direction,
                                          Gtk::StateType state, IconSize size,
                                          Widget& widget, const Glib::ustring& detail);

  
  /** Icon sets have a list of Gtk::IconSource, which they use as base
   * icons for rendering icons in different states and sizes. Icons are
   * scaled, made to look insensitive, etc. in
   * gtk_icon_set_render_icon(), but Gtk::IconSet needs base images to
   * work with. The base images and when to use them are described by
   * a Gtk::IconSource.
   * 
   * This function copies @a source , so you can reuse the same source immediately
   * without affecting the icon set.
   * 
   * An example of when you'd use this function: a web browser's "Back
   * to Previous Page" icon might point in a different direction in
   * Hebrew and in English; it might look different when insensitive;
   * and it might change size depending on toolbar mode (small/large
   * icons). So a single icon set would contain all those variants of
   * the icon, and you might add a separate source for each one.
   * 
   * You should nearly always add a "default" icon source with all
   * fields wildcarded, which will be used as a fallback if no more
   * specific source matches. Gtk::IconSet always prefers more specific
   * icon sources to more generic icon sources. The order in which you
   * add the sources to the icon set does not matter.
   * 
   * gtk_icon_set_new_from_pixbuf() creates a new icon set with a
   * default icon source based on the given pixbuf.
   * @param source A Gtk::IconSource.
   */
  void add_source(const IconSource& source);

  Glib::ArrayHandle<IconSize> get_sizes() const;

  static IconSet lookup_default(const Gtk::StockID& stock_id);


};

} /* namespace Gtk */


namespace Gtk
{

/** @relates Gtk::IconSet
 * @param lhs The left-hand side
 * @param rhs The right-hand side
 */
inline void swap(IconSet& lhs, IconSet& rhs)
  { lhs.swap(rhs); }

} // namespace Gtk

namespace Glib
{

/** @relates Gtk::IconSet
 * @param object The C instance
 * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
 * @result A C++ instance that wraps this C instance.
 */
Gtk::IconSet wrap(GtkIconSet* object, bool take_copy = false);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <>
class Value<Gtk::IconSet> : public Glib::Value_Boxed<Gtk::IconSet>
{};
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

} // namespace Glib


#endif /* _GTKMM_ICONSET_H */

