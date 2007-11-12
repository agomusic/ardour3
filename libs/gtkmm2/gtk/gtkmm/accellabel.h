// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_ACCELLABEL_H
#define _GTKMM_ACCELLABEL_H


#include <glibmm.h>

/* $Id$ */

/* accellabel.h
 * 
 * Copyright (C) 1998-2002 The gtkmm Development Team
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

#include <gtkmm/label.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkAccelLabel GtkAccelLabel;
typedef struct _GtkAccelLabelClass GtkAccelLabelClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class AccelLabel_Class; } // namespace Gtk
namespace Gtk
{

/** A label which displays an accelerator key on the right of the text.
 * Used for menu item labels, for instance.
 *
 * @ingroup Widgets
 */

class AccelLabel : public Label
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef AccelLabel CppObjectType;
  typedef AccelLabel_Class CppClassType;
  typedef GtkAccelLabel BaseObjectType;
  typedef GtkAccelLabelClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~AccelLabel();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class AccelLabel_Class;
  static CppClassType accellabel_class_;

  // noncopyable
  AccelLabel(const AccelLabel&);
  AccelLabel& operator=(const AccelLabel&);

protected:
  explicit AccelLabel(const Glib::ConstructParams& construct_params);
  explicit AccelLabel(GtkAccelLabel* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkAccelLabel*       gobj()       { return reinterpret_cast<GtkAccelLabel*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkAccelLabel* gobj() const { return reinterpret_cast<GtkAccelLabel*>(gobject_); }


public:
  //C++ methods used to invoke GTK+ virtual functions:
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

protected:
  //GTK+ Virtual Functions (override these to change behaviour):
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

  //Default Signal Handlers::
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


private:

  
public:

  //The default ctor doesn't correspond to any _new function, but gtkmenuitem.cc does a simple g_object_new() with no properties.
  /** Default constructor to create an AccelLabel object
   */
  AccelLabel();

  /** Constructor to create an AccelLabel object with a default label
   * @param label The label string.
   * @param mnemonic If true, characters preceded by an underscore (_) will be underlined and used as a keyboard accelerator.
   */
  explicit AccelLabel(const Glib::ustring& label, bool mnemonic = false);

  
  /** Sets the widget to be monitored by this accelerator label.
   * @param accel_widget The widget to be monitored.
   */
  void set_accel_widget(const Widget& accel_widget);
  /// Forget the accel widget previously set by set_accel_widget().
  void unset_accel_widget();

  
  /** Fetches the widget monitored by this accelerator label. See
   * set_accel_widget().
   * @return The object monitored by the accelerator label,
   * or <tt>0</tt>.
   */
  Widget* get_accel_widget();
  
  /** Fetches the widget monitored by this accelerator label. See
   * set_accel_widget().
   * @return The object monitored by the accelerator label,
   * or <tt>0</tt>.
   */
  const Widget* get_accel_widget() const;

  
  /** Gets the width needed to display this accelerator label. This is used by menus to align all of the Gtk::MenuItem widgets, and shouldn't be needed by applications.
   * @return Width of this accelerator label.
   */
  guint get_accel_width() const;
  
  /** Recreates the string representing the accelerator keys.
   * @return Always returns <tt>false</tt>.
   */
  bool refetch();

  //_WRAP_PROPERTY("accel-closure", Glib::Object) //GClosure
  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The widget to be monitored for accelerator changes.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Gtk::Widget*> property_accel_widget() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The widget to be monitored for accelerator changes.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Gtk::Widget*> property_accel_widget() const;
#endif //#GLIBMM_PROPERTIES_ENABLED


};

} /* namespace Gtk */


namespace Glib
{
  /** @relates Gtk::AccelLabel
   * @param object The C instance
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   */
  Gtk::AccelLabel* wrap(GtkAccelLabel* object, bool take_copy = false);
} //namespace Glib


#endif /* _GTKMM_ACCELLABEL_H */

