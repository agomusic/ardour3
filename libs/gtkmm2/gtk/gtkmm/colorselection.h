// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_COLORSELECTION_H
#define _GTKMM_COLORSELECTION_H


#include <glibmm.h>

/* $Id$ */

/* Copyright (C) 1998-2002 The gtkmm Development Team
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


#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/button.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkColorSelection GtkColorSelection;
typedef struct _GtkColorSelectionClass GtkColorSelectionClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class ColorSelection_Class; } // namespace Gtk
#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkColorSelectionDialog GtkColorSelectionDialog;
typedef struct _GtkColorSelectionDialogClass GtkColorSelectionDialogClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class ColorSelectionDialog_Class; } // namespace Gtk
namespace Gtk
{

/** A widget used to select a color.
 *
 * This widget is used to select a color. It
 * consists of a color wheel and number of sliders and entry boxes for color
 * parameters such as hue, saturation, value, red, green, blue, and opacity.
 *
 * It is found on the standard color selection dialog box
 * Gtk::ColorSelectionDialog. 
 *
 * @ingroup Widgets
 */

class ColorSelection : public VBox
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef ColorSelection CppObjectType;
  typedef ColorSelection_Class CppClassType;
  typedef GtkColorSelection BaseObjectType;
  typedef GtkColorSelectionClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~ColorSelection();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class ColorSelection_Class;
  static CppClassType colorselection_class_;

  // noncopyable
  ColorSelection(const ColorSelection&);
  ColorSelection& operator=(const ColorSelection&);

protected:
  explicit ColorSelection(const Glib::ConstructParams& construct_params);
  explicit ColorSelection(GtkColorSelection* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkColorSelection*       gobj()       { return reinterpret_cast<GtkColorSelection*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkColorSelection* gobj() const { return reinterpret_cast<GtkColorSelection*>(gobject_); }


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
  virtual void on_color_changed();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


private:

  
public:
  ColorSelection();

  
  /** Determines whether the colorsel has an opacity control.
   * @return <tt>true</tt> if the @a colorsel  has an opacity control.  <tt>false</tt> if it does't.
   */
  bool get_has_opacity_control() const;
  
  /** Sets the @a colorsel  to use or not use opacity.
   * @param has_opacity <tt>true</tt> if @a colorsel  can set the opacity, <tt>false</tt> otherwise.
   */
  void set_has_opacity_control(bool has_opacity = true);
  
  /** Determines whether the color selector has a color palette.
   * @return <tt>true</tt> if the selector has a palette.  <tt>false</tt> if it hasn't.
   */
  bool get_has_palette() const;
  
  /** Shows and hides the palette based upon the value of @a has_palette .
   * @param has_palette <tt>true</tt> if palette is to be visible, <tt>false</tt> otherwise.
   */
  void set_has_palette(bool has_palette = true);
  
  /** Sets the current color to be @a color .  The first time this is called, it will
   * also set the original color to be @a color  too.
   * @param color A Gdk::Color to set the current color with.
   */
  void set_current_color(const Gdk::Color& color);
  
  /** Sets the current opacity to be @a alpha .  The first time this is called, it will
   * also set the original opacity to be @a alpha  too.
   * @param alpha An integer between 0 and 65535.
   */
  void set_current_alpha(guint16 alpha);
  Gdk::Color get_current_color() const;
  
  /** Return value: an integer between 0 and 65535.
   * @return An integer between 0 and 65535.
   */
  guint16 get_current_alpha() const;
  
  /** Sets the 'previous' color to be @a color .  This function should be called with
   * some hesitations, as it might seem confusing to have that color change.
   * Calling set_current_color() will also set this color the first
   * time it is called.
   * @param color A Gdk::Color to set the previous color with.
   */
  void set_previous_color(const Gdk::Color& color);
  
  /** Sets the 'previous' alpha to be @a alpha .  This function should be called with
   * some hesitations, as it might seem confusing to have that alpha change.
   * @param alpha An integer between 0 and 65535.
   */
  void set_previous_alpha(guint16 alpha);
  Gdk::Color get_previous_color() const;
  
  /** Return value: an integer between 0 and 65535.
   * @return An integer between 0 and 65535.
   */
  guint16 get_previous_alpha() const;

  
  /** Gets the current state of the @a colorsel .
   * @return <tt>true</tt> if the user is currently dragging a color around, and <tt>false</tt>
   * if the selection has stopped.
   */
  bool is_adjusting() const;

  static Gdk::ArrayHandle_Color palette_from_string(const Glib::ustring& str);
  static Glib::ustring palette_to_string(const Gdk::ArrayHandle_Color& colors);

  typedef sigc::slot<void, const Glib::RefPtr<Gdk::Screen>&,
                            const Gdk::ArrayHandle_Color&> SlotChangePaletteHook;

  static SlotChangePaletteHook set_change_palette_hook(const SlotChangePaletteHook& slot);

  
  /**
   * @par Prototype:
   * <tt>void on_my_%color_changed()</tt>
   */

  Glib::SignalProxy0< void > signal_color_changed();


  #ifdef GLIBMM_PROPERTIES_ENABLED
/** Whether a palette should be used.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_has_palette() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** Whether a palette should be used.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_has_palette() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** Whether the color selector should allow setting opacity.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_has_opacity_control() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** Whether the color selector should allow setting opacity.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_has_opacity_control() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The current color.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Gdk::Color> property_current_color() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The current color.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Gdk::Color> property_current_color() const;
#endif //#GLIBMM_PROPERTIES_ENABLED

  #ifdef GLIBMM_PROPERTIES_ENABLED
/** The current opacity value (0 fully transparent, 65535 fully opaque).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<guint> property_current_alpha() ;
#endif //#GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
/** The current opacity value (0 fully transparent, 65535 fully opaque).
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<guint> property_current_alpha() const;
#endif //#GLIBMM_PROPERTIES_ENABLED


};

/** This dialog allows the user to select a color.
 * @ingroup Dialogs
 */

class ColorSelectionDialog : public Dialog
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef ColorSelectionDialog CppObjectType;
  typedef ColorSelectionDialog_Class CppClassType;
  typedef GtkColorSelectionDialog BaseObjectType;
  typedef GtkColorSelectionDialogClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~ColorSelectionDialog();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class ColorSelectionDialog_Class;
  static CppClassType colorselectiondialog_class_;

  // noncopyable
  ColorSelectionDialog(const ColorSelectionDialog&);
  ColorSelectionDialog& operator=(const ColorSelectionDialog&);

protected:
  explicit ColorSelectionDialog(const Glib::ConstructParams& construct_params);
  explicit ColorSelectionDialog(GtkColorSelectionDialog* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkColorSelectionDialog*       gobj()       { return reinterpret_cast<GtkColorSelectionDialog*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkColorSelectionDialog* gobj() const { return reinterpret_cast<GtkColorSelectionDialog*>(gobject_); }


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

  ColorSelectionDialog();
  explicit ColorSelectionDialog(const Glib::ustring& title);

   ColorSelection* get_colorsel();
  const ColorSelection* get_colorsel() const;
    Button* get_ok_button();
  const Button* get_ok_button() const;
    Button* get_cancel_button();
  const Button* get_cancel_button() const;
    Button* get_help_button();
  const Button* get_help_button() const;
 

};

} // namespace Gtk


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gtk::ColorSelection
   */
  Gtk::ColorSelection* wrap(GtkColorSelection* object, bool take_copy = false);
} //namespace Glib


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gtk::ColorSelectionDialog
   */
  Gtk::ColorSelectionDialog* wrap(GtkColorSelectionDialog* object, bool take_copy = false);
} //namespace Glib


#endif /* _GTKMM_COLORSELECTION_H */

