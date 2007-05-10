// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_ACTION_H
#define _GTKMM_ACTION_H

#include <glibmm.h>

/* $Id$ */

/* Copyright (C) 2003 The gtkmm Development Team
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

#include <gtkmm/widget.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/stockid.h>
 

#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkAction GtkAction;
typedef struct _GtkActionClass GtkActionClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class Action_Class; } // namespace Gtk
namespace Gtk
{

class MenuItem;
class ToolItem;
class Image;


class Action : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef Action CppObjectType;
  typedef Action_Class CppClassType;
  typedef GtkAction BaseObjectType;
  typedef GtkActionClass BaseClassType;

private:  friend class Action_Class;
  static CppClassType action_class_;

private:
  // noncopyable
  Action(const Action&);
  Action& operator=(const Action&);

protected:
  explicit Action(const Glib::ConstructParams& construct_params);
  explicit Action(GtkAction* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~Action();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GtkAction*       gobj()       { return reinterpret_cast<GtkAction*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GtkAction* gobj() const { return reinterpret_cast<GtkAction*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GtkAction* gobj_copy();

private:

 
protected:
 Action();
  explicit Action(const Glib::ustring& name, const StockID& stock_id, const Glib::ustring& label = Glib::ustring(), const Glib::ustring& tooltip = Glib::ustring());

public:
  
  static Glib::RefPtr<Action> create();

  static Glib::RefPtr<Action> create(const Glib::ustring& name, const Glib::ustring& label =  Glib::ustring(), const Glib::ustring& tooltip =  Glib::ustring());
  static Glib::RefPtr<Action> create(const Glib::ustring& name, const Gtk::StockID& stock_id, const Glib::ustring& label =  Glib::ustring(), const Glib::ustring& tooltip =  Glib::ustring());

  
  /** Returns the name of the action.
   * @return The name of the action. The string belongs to GTK+ and should not
   * be freed.
   * 
   * Since: 2.4.
   */
  Glib::ustring get_name() const;

  
  /** Returns whether the action is effectively sensitive.
   * @return <tt>true</tt> if the action and its associated action group 
   * are both sensitive.
   * 
   * Since: 2.4.
   */
  bool is_sensitive() const;
  
  /** Returns whether the action itself is sensitive. Note that this doesn't 
   * necessarily mean effective sensitivity. See is_sensitive() 
   * for that.
   * @return <tt>true</tt> if the action itself is sensitive.
   * 
   * Since: 2.4.
   */
  bool get_sensitive() const;

   //TODO: Just wrap gtk_action_set_sensitive() when they put it in GTK+.
   void set_tooltip(const Glib::ustring& tooltip);

  
  /** Sets the ::sensitive property of the action to @a sensitive . Note that 
   * this doesn't necessarily mean effective sensitivity. See 
   * is_sensitive() 
   * for that.
   * 
   * Since: 2.6
   * @param sensitive <tt>true</tt> to make the action sensitive.
   */
  void set_sensitive(bool sensitive = true);
  
  
  /** Returns whether the action is effectively visible.
   * @return <tt>true</tt> if the action and its associated action group 
   * are both visible.
   * 
   * Since: 2.4.
   */
  bool is_visible() const;
  
  /** Returns whether the action itself is visible. Note that this doesn't 
   * necessarily mean effective visibility. See is_sensitive() 
   * for that.
   * @return <tt>true</tt> if the action itself is visible.
   * 
   * Since: 2.4.
   */
  bool get_visible() const;
  
  /** Sets the ::visible property of the action to @a visible . Note that 
   * this doesn't necessarily mean effective visibility. See 
   * is_visible() 
   * for that.
   * 
   * Since: 2.6
   * @param visible <tt>true</tt> to make the action visible.
   */
  void set_visible(bool visible = true);

  
  /** Emits the "activate" signal on the specified action, if it isn't 
   * insensitive. This gets called by the proxy widgets when they get 
   * activated.
   * 
   * It can also be used to manually activate an action.
   * 
   * Since: 2.4
   */
  void activate();
  
  /** This function is intended for use by action implementations to
   * create icons displayed in the proxy widgets.
   * @param icon_size The size of the icon that should be created.
   * @return A widget that displays the icon for this action.
   * 
   * Since: 2.4.
   */
  Image* create_icon(IconSize icon_size);
  
  /** Creates a menu item widget that proxies for the given action.
   * @return A menu item connected to the action.
   * 
   * Since: 2.4.
   */
  MenuItem* create_menu_item();
  
  /** Creates a toolbar item widget that proxies for the given action.
   * @return A toolbar item connected to the action.
   * 
   * Since: 2.4.
   */
  ToolItem* create_tool_item();
  
  /** Connects a widget to an action object as a proxy.  Synchronises 
   * various properties of the action with the widget (such as label 
   * text, icon, tooltip, etc), and attaches a callback so that the 
   * action gets activated when the proxy widget does.
   * 
   * If the widget is already connected to an action, it is disconnected
   * first.
   * 
   * Since: 2.4
   * @param proxy The proxy widget.
   */
  void connect_proxy(Widget& proxy);
  
  /** Disconnects a proxy widget from an action.  
   * Does <em>not</em> destroy the widget, however.
   * 
   * Since: 2.4
   * @param proxy The proxy widget.
   */
  void disconnect_proxy(Widget& proxy);
  
  /** Returns the proxy widgets for an action.
   * @return A G::SList of proxy widgets. The list is owned by the action and
   * must not be modified.
   * 
   * Since: 2.4.
   */
  Glib::SListHandle<Widget*> get_proxies();
  
  /** Returns the proxy widgets for an action.
   * @return A G::SList of proxy widgets. The list is owned by the action and
   * must not be modified.
   * 
   * Since: 2.4.
   */
  Glib::SListHandle<const Widget*> get_proxies() const;
  
  /** Installs the accelerator for @a action  if @a action  has an
   * accel path and group. See set_accel_path() and 
   * set_accel_group()
   * 
   * Since multiple proxies may independently trigger the installation
   * of the accelerator, the @a action  counts the number of times this
   * function has been called and doesn't remove the accelerator until
   * disconnect_accelerator() has been called as many times.
   * 
   * Since: 2.4
   */
  void connect_accelerator();
  
  /** Undoes the effect of one call to connect_accelerator().
   * 
   * Since: 2.4
   */
  void disconnect_accelerator();
  
  
  /** Returns the accel path for this action.  
   * 
   * Since: 2.6
   * @return The accel path for this action, or <tt>0</tt>
   * if none is set. The returned string is owned by GTK+
   * and must not be freed or modified.
   */
  Glib::ustring get_accel_path() const;

  /// For instance, void on_activate();
  typedef sigc::slot<void> SlotActivate;
  

  Glib::SignalProxy0< void > signal_activate();


  //Used by AccelGroup:
  
  /** Sets the accel path for this action.  All proxy widgets associated
   * with the action will have this accel path, so that their
   * accelerators are consistent.
   * 
   * Since: 2.4
   * @param accel_path The accelerator path.
   */
  void set_accel_path(const Glib::ustring& accel_path);
  
  /** Sets the Gtk::AccelGroup in which the accelerator for this action
   * will be installed.
   * 
   * Since: 2.4
   * @param accel_group A Gtk::AccelGroup or <tt>0</tt>.
   */
  void set_accel_group(const Glib::RefPtr<AccelGroup>& accel_group);

  /** A unique name for the action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_name() const;


  /** The label used for menu items and buttons that activate this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_label() ;

/** The label used for menu items and buttons that activate this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_label() const;

  /** A shorter label that may be used on toolbar buttons.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_short_label() ;

/** A shorter label that may be used on toolbar buttons.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_short_label() const;

  /** A tooltip for this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_tooltip() ;

/** A tooltip for this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_tooltip() const;

  /** The stock icon displayed in widgets representing this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<StockID> property_stock_id() ;

/** The stock icon displayed in widgets representing this action.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<StockID> property_stock_id() const;

  /** Whether the toolbar item is visible when the toolbar is in a horizontal orientation.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_visible_horizontal() ;

/** Whether the toolbar item is visible when the toolbar is in a horizontal orientation.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_visible_horizontal() const;

  /** Whether the toolbar item is visible when the toolbar is in a vertical orientation.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_visible_vertical() ;

/** Whether the toolbar item is visible when the toolbar is in a vertical orientation.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_visible_vertical() const;

  /** Whether the action is considered important. When TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_is_important() ;

/** Whether the action is considered important. When TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_is_important() const;

  /** When TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_hide_if_empty() ;

/** When TRUE
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_hide_if_empty() const;

  /** Whether the action is enabled.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_sensitive() ;

/** Whether the action is enabled.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_sensitive() const;

  /** Whether the action is visible.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<bool> property_visible() ;

/** Whether the action is visible.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<bool> property_visible() const;


protected:
  //For use by child actions:  
  
  /** Disables calls to the activate()
   * function by signals on the given proxy widget.  This is used to
   * break notification loops for things like check or radio actions.
   * 
   * This function is intended for use by action implementations.
   * 
   * Since: 2.4
   * @param proxy A proxy widget.
   */
  void block_activate_from(Widget& proxy);
  
  /** Re-enables calls to the activate()
   * function by signals on the given proxy widget.  This undoes the
   * blocking done by block_activate_from().
   * 
   * This function is intended for use by action implementations.
   * 
   * Since: 2.4
   * @param proxy A proxy widget.
   */
  void unblock_activate_from(Widget& proxy);


protected: 
  //Widget-creation routines:
    virtual Widget* create_menu_item_vfunc();
    virtual Widget* create_tool_item_vfunc();
    virtual void connect_proxy_vfunc(Widget* proxy);
    virtual void disconnect_proxy_vfunc(Widget* proxy);


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::
  virtual void on_activate();


};

} // namespace Gtk


namespace Glib
{
  /** @relates Gtk::Action
   * @param object The C instance
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   */
  Glib::RefPtr<Gtk::Action> wrap(GtkAction* object, bool take_copy = false);
}


#endif /* _GTKMM_ACTION_H */

