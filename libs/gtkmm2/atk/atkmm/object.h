// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _ATKMM_OBJECT_H
#define _ATKMM_OBJECT_H

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


#include <atkmm/component.h>
#include <atkmm/relation.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _AtkPropertyValues AtkPropertyValues; }
#endif


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _AtkObject AtkObject;
typedef struct _AtkObjectClass AtkObjectClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Atk
{ class Object_Class; } // namespace Atk
namespace Atk
{


/** @addtogroup atkmmEnums Enums and Flags */

/**
 * @ingroup atkmmEnums
 */
enum Role
{
  ROLE_INVALID,
  ROLE_ACCEL_LABEL,
  ROLE_ALERT,
  ROLE_ANIMATION,
  ROLE_ARROW,
  ROLE_CALENDAR,
  ROLE_CANVAS,
  ROLE_CHECK_BOX,
  ROLE_CHECK_MENU_ITEM,
  ROLE_COLOR_CHOOSER,
  ROLE_COLUMN_HEADER,
  ROLE_COMBO_BOX,
  ROLE_DATE_EDITOR,
  ROLE_DESKTOP_ICON,
  ROLE_DESKTOP_FRAME,
  ROLE_DIAL,
  ROLE_DIALOG,
  ROLE_DIRECTORY_PANE,
  ROLE_DRAWING_AREA,
  ROLE_FILE_CHOOSER,
  ROLE_FILLER,
  ROLE_FONT_CHOOSER,
  ROLE_FRAME,
  ROLE_GLASS_PANE,
  ROLE_HTML_CONTAINER,
  ROLE_ICON,
  ROLE_IMAGE,
  ROLE_INTERNAL_FRAME,
  ROLE_LABEL,
  ROLE_LAYERED_PANE,
  ROLE_LIST,
  ROLE_LIST_ITEM,
  ROLE_MENU,
  ROLE_MENU_BAR,
  ROLE_MENU_ITEM,
  ROLE_OPTION_PANE,
  ROLE_PAGE_TAB,
  ROLE_PAGE_TAB_LIST,
  ROLE_PANEL,
  ROLE_PASSWORD_TEXT,
  ROLE_POPUP_MENU,
  ROLE_PROGRESS_BAR,
  ROLE_PUSH_BUTTON,
  ROLE_RADIO_BUTTON,
  ROLE_RADIO_MENU_ITEM,
  ROLE_ROOT_PANE,
  ROLE_ROW_HEADER,
  ROLE_SCROLL_BAR,
  ROLE_SCROLL_PANE,
  ROLE_SEPARATOR,
  ROLE_SLIDER,
  ROLE_SPLIT_PANE,
  ROLE_SPIN_BUTTON,
  ROLE_STATUSBAR,
  ROLE_TABLE,
  ROLE_TABLE_CELL,
  ROLE_TABLE_COLUMN_HEADER,
  ROLE_TABLE_ROW_HEADER,
  ROLE_TEAR_OFF_MENU_ITEM,
  ROLE_TERMINAL,
  ROLE_TEXT,
  ROLE_TOGGLE_BUTTON,
  ROLE_TOOL_BAR,
  ROLE_TOOL_TIP,
  ROLE_TREE,
  ROLE_TREE_TABLE,
  ROLE_UNKNOWN,
  ROLE_VIEWPORT,
  ROLE_WINDOW,
  ROLE_HEADER,
  ROLE_FOOTER,
  ROLE_PARAGRAPH,
  ROLE_RULER,
  ROLE_APPLICATION,
  ROLE_AUTOCOMPLETE,
  ROLE_EDITBAR,
  ROLE_EMBEDDED,
  ROLE_LAST_DEFINED
};

} // namespace Atk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Atk::Role> : public Glib::Value_Enum<Atk::Role>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Atk
{


class RelationSet;
class Relation;
class StateSet;

typedef guint64 State;

/** The base object class for the Accessibility Toolkit API.
 * This class is the primary class for accessibility support via the Accessibility ToolKit (ATK). Objects which are
 * instances of Atk::Object (or instances of Atk::Object-derived types) are queried for properties which relate basic
 * (and generic) properties of a UI component such as name and description. Instances of Atk::Object may also be queried
 * as to whether they implement other ATK interfaces (e.g. Atk::Action, Atk::Component, etc.), as appropriate to the role
 * which a given UI component plays in a user interface.
 *
 * All UI components in an application which provide useful information or services to the user must provide corresponding
 * Atk::Object instances on request (in GTK+, for instance, usually on a call to Gtk::Widget::get_accessible()), either via
 * ATK support built into the toolkit for the widget class or ancestor class, or in the case of custom widgets, if the
 * inherited Atk::Object implementation is insufficient, via instances of a new Atk::Object subclass. 
 */

class Object : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef Object CppObjectType;
  typedef Object_Class CppClassType;
  typedef AtkObject BaseObjectType;
  typedef AtkObjectClass BaseClassType;

private:  friend class Object_Class;
  static CppClassType object_class_;

private:
  // noncopyable
  Object(const Object&);
  Object& operator=(const Object&);

protected:
  explicit Object(const Glib::ConstructParams& construct_params);
  explicit Object(AtkObject* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~Object();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  AtkObject*       gobj()       { return reinterpret_cast<AtkObject*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const AtkObject* gobj() const { return reinterpret_cast<AtkObject*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  AtkObject* gobj_copy();

private:

   // see wrap_new() implementation in object.ccg
  
public:

  
  /** Gets the accessible name of the accessible.
   * @return A character string representing the accessible name of the object.
   */
  Glib::ustring get_name() const;
  
  /** Gets the accessible description of the accessible.
   * @return A character string representing the accessible description
   * of the accessible.
   */
  Glib::ustring get_description() const;
  
  /** Gets the accessible parent of the accessible.
   * @return A Atk::Object representing the accessible parent of the accessible.
   */
  Glib::RefPtr<Atk::Object> get_parent();
  
  /** Gets the number of accessible children of the accessible.
   * @return An integer representing the number of accessible children
   * of the accessible.
   */
  int get_n_accessible_children() const;
  
  /** Gets a reference to the specified accessible child of the object.
   * The accessible children are 0-based so the first accessible child is
   * at index 0, the second at index 1 and so on.
   * @param i A <tt>int</tt> representing the position of the child, starting from 0.
   * @return An Atk::Object representing the specified accessible child
   * of the accessible.
   */
  Glib::RefPtr<Atk::Object> get_accessible_child(int i);
  
  /** Gets the Atk::RelationSet associated with the object.
   * @return An Atk::RelationSet representing the relation set of the object.
   */
  Glib::RefPtr<RelationSet> get_relation_set();
  
  /** Gets the role of the accessible.
   * @return An Atk::Role which is the role of the accessible.
   */
  Role get_role() const;
  
  /** Gets a reference to the state set of the accessible; the caller must
   * unreference it when it is no longer needed.
   * @return A reference to an Atk::StateSet which is the state
   * set of the accessible.
   */
  Glib::RefPtr<StateSet> get_state_set();
  
  /** Gets the 0-based index of this accessible in its parent; returns -1 if the
   * accessible does not have an accessible parent.
   * @return An integer which is the index of the accessible in its parent.
   */
  int get_index_in_parent();
  
  /** Sets the accessible name of the accessible.
   * @param name A character string to be set as the accessible name.
   */
  void set_name(const Glib::ustring& name);
  
  /** Sets the accessible description of the accessible.
   * @param description A character string to be set as the accessible description.
   */
  void set_description(const Glib::ustring& description);
  
  /** Sets the accessible parent of the accessible.
   * @param parent An Atk::Object to be set as the accessible parent.
   */
  void set_parent(const Glib::RefPtr<Atk::Object>& parent);
  
  /** Sets the role of the accessible.
   * @param role An Atk::Role to be set as the role.
   */
  void set_role(Role role);
  //_WRAP_METHOD(guint connect_property_change_handler(AtkPropertyChangeHandler* handler), atk_object_connect_property_change_handler)
  //_WRAP_METHOD(void remove_property_change_handler(guint handler_id), atk_object_remove_property_change_handler)
  
  /** Emits a state-change signal for the specified state.
   * @param state An Atk::State whose state is changed.
   * @param value A <tt>bool</tt> which indicates whether the state is being set on or off.
   */
  void notify_state_change(State state, bool value);

  
  /** Adds a relationship of the specified type with the specified target.
   * @param relationship The Atk::RelationType of the relation.
   * @param target The Atk::Object which is to be the target of the relation.
   * @return <tt>true</tt> if the relationship is added.
   */
  bool add_relationship(RelationType relationship, const Glib::RefPtr<Object>& target);
  
  /** Removes a relationship of the specified type with the specified target.
   * @param relationship The Atk::RelationType of the relation.
   * @param target The Atk::Object which is the target of the relation to be removed.
   * @return <tt>true</tt> if the relationship is removed.
   */
  bool remove_relationship(RelationType relationship, const Glib::RefPtr<Object>& target);

  
  Glib::SignalProxy2< void,guint,gpointer > signal_children_changed();

  
  Glib::SignalProxy1< void,bool > signal_focus_event();

  
  Glib::SignalProxy1< void,AtkPropertyValues* > signal_property_change();

  
  Glib::SignalProxy2< void,const Glib::ustring&,bool > signal_state_change();

  
  Glib::SignalProxy0< void > signal_visible_data_changed();

  
  Glib::SignalProxy1< void,void** > signal_active_descendant_changed();


  /** Object instance's name formatted for assistive technology access.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_accessible_name() ;

/** Object instance's name formatted for assistive technology access.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_accessible_name() const;

  /** Description of an object
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_accessible_description() ;

/** Description of an object
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_accessible_description() const;

  /** Is used to notify that the parent has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy< Glib::RefPtr<Atk::Object> > property_accessible_parent() ;

/** Is used to notify that the parent has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< Glib::RefPtr<Atk::Object> > property_accessible_parent() const;

  /** Is used to notify that the value has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<double> property_accessible_value() ;

/** Is used to notify that the value has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<double> property_accessible_value() const;

  /** The accessible role of this object.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<int> property_accessible_role() ;

/** The accessible role of this object.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_accessible_role() const;

  /** The accessible layer of this object.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_accessible_component_layer() const;


  /** The accessible MDI value of this object.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<int> property_accessible_component_mdi_zorder() const;


  /** Is used to notify that the table caption has changed; this property should not be used. accessible-table-caption-object should be used instead.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_accessible_table_caption() ;

/** Is used to notify that the table caption has changed; this property should not be used. accessible-table-caption-object should be used instead.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_accessible_table_caption() const;

  /** Is used to notify that the table column description has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_accessible_table_column_description() ;

/** Is used to notify that the table column description has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_accessible_table_column_description() const;

  /** Is used to notify that the table column header has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy< Glib::RefPtr<Atk::Object> > property_accessible_table_column_header() ;

/** Is used to notify that the table column header has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< Glib::RefPtr<Atk::Object> > property_accessible_table_column_header() const;

  /** Is used to notify that the table row description has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy<Glib::ustring> property_accessible_table_row_description() ;

/** Is used to notify that the table row description has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly<Glib::ustring> property_accessible_table_row_description() const;

  /** Is used to notify that the table row header has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy< Glib::RefPtr<Atk::Object> > property_accessible_table_row_header() ;

/** Is used to notify that the table row header has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< Glib::RefPtr<Atk::Object> > property_accessible_table_row_header() const;

  /** Is used to notify that the table summary has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy< Glib::RefPtr<Atk::Object> > property_accessible_table_summary() ;

/** Is used to notify that the table summary has changed.
   *
   * You rarely need to use properties because there are get_ and set_ methods for almost all of them.
   * @return A PropertyProxy that allows you to get or set the property of the value, or receive notification when
   * the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< Glib::RefPtr<Atk::Object> > property_accessible_table_summary() const;


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::
  virtual void on_children_changed(guint change_index, gpointer changed_child);
  virtual void on_focus_event(bool focus_in);
  virtual void on_property_change(AtkPropertyValues* values);
  virtual void on_state_change(const Glib::ustring& name, bool state_set);
  virtual void on_visible_data_changed();
  virtual void on_active_descendant_changed(void** child);


};

} // namespace Atk


namespace Glib
{
  /** @relates Atk::Object
   * @param object The C instance
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   */
  Glib::RefPtr<Atk::Object> wrap(AtkObject* object, bool take_copy = false);
}


#endif /* _ATKMM_OBJECT_H */

