// Generated by gtkmmproc -- DO NOT MODIFY!

#include <gtkmm/treedragsource.h>
#include <gtkmm/private/treedragsource_p.h>

// -*- c++ -*-
/* $Id$ */

/* Copyright 1998-2002 The gtkmm Development Team
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

#include <gtkmm/treepath.h>
#include <gtkmm/selectiondata_private.h> //For SelectionData_WithoutOwnership
#include <gtk/gtktreednd.h>


namespace Gtk
{

//This vfunc wrapper is manually written, so that we can use a temporary instance for the SelectionData& output parameter:

gboolean TreeDragSource_Class::drag_data_get_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path, GtkSelectionData* selection_data)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      SelectionData_WithoutOwnership temp_instance(selection_data);
      return static_cast<int>(obj->drag_data_get_vfunc(Gtk::TreePath(path, true), temp_instance));
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->drag_data_get)
      return (*base->drag_data_get)(self, path, selection_data);
  }

  typedef gboolean RType;
  return RType();
}

bool Gtk::TreeDragSource::drag_data_get_vfunc(const TreeModel::Path& path, SelectionData& selection_data) const
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->drag_data_get)
    return (*base->drag_data_get)(const_cast<GtkTreeDragSource*>(gobj()),const_cast<GtkTreePath*>((path).gobj()), selection_data.gobj());

  typedef bool RType;
  return RType();
}

} //namespace Gtk


namespace
{
} // anonymous namespace


namespace Glib
{

Glib::RefPtr<Gtk::TreeDragSource> wrap(GtkTreeDragSource* object, bool take_copy)
{
  return Glib::RefPtr<Gtk::TreeDragSource>( dynamic_cast<Gtk::TreeDragSource*> (Glib::wrap_auto ((GObject*)(object), take_copy)) );
  //We use dynamic_cast<> in case of multiple inheritance.
}

} // namespace Glib


namespace Gtk
{


/* The *_Class implementation: */

const Glib::Interface_Class& TreeDragSource_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Interface_Class has to know the interface init function
    // in order to add interfaces to implementing types.
    class_init_func_ = &TreeDragSource_Class::iface_init_function;

    // We can not derive from another interface, and it is not necessary anyway.
    gtype_ = gtk_tree_drag_source_get_type();
  }

  return *this;
}

void TreeDragSource_Class::iface_init_function(void* g_iface, void*)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_iface);

  //This is just to avoid an "unused variable" warning when there are no vfuncs or signal handlers to connect.
  //This is a temporary fix until I find out why I can not seem to derive a GtkFileChooser interface. murrayc
  g_assert(klass != 0); 

    klass->drag_data_get = &drag_data_get_vfunc_callback;
    klass->row_draggable = &row_draggable_vfunc_callback;
  klass->drag_data_delete = &drag_data_delete_vfunc_callback;
}

gboolean TreeDragSource_Class::row_draggable_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      return static_cast<int>(obj->row_draggable_vfunc(Gtk::TreePath(path, true)
));
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->row_draggable)
      return (*base->row_draggable)(self, path);
  }

  typedef gboolean RType;
  return RType();
}

gboolean TreeDragSource_Class::drag_data_delete_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      return static_cast<int>(obj->drag_data_delete_vfunc(Gtk::TreePath(path, true)
));
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->drag_data_delete)
      return (*base->drag_data_delete)(self, path);
  }

  typedef gboolean RType;
  return RType();
}


Glib::ObjectBase* TreeDragSource_Class::wrap_new(GObject* object)
{
  return new TreeDragSource((GtkTreeDragSource*)(object));
}


/* The implementation: */

TreeDragSource::TreeDragSource()
:
  Glib::Interface(treedragsource_class_.init())
{}

TreeDragSource::TreeDragSource(GtkTreeDragSource* castitem)
:
  Glib::Interface((GObject*)(castitem))
{}

TreeDragSource::~TreeDragSource()
{}

// static
void TreeDragSource::add_interface(GType gtype_implementer)
{
  treedragsource_class_.init().add_interface(gtype_implementer);
}

TreeDragSource::CppClassType TreeDragSource::treedragsource_class_; // initialize static member

GType TreeDragSource::get_type()
{
  return treedragsource_class_.init().get_type();
}

GType TreeDragSource::get_base_type()
{
  return gtk_tree_drag_source_get_type();
}


bool TreeDragSource::row_draggable(const TreeModel::Path& path) const
{
  return gtk_tree_drag_source_row_draggable(const_cast<GtkTreeDragSource*>(gobj()), const_cast<GtkTreePath*>((path).gobj()));
}

bool TreeDragSource::drag_data_get(const TreeModel::Path& path, SelectionData& selection_data)
{
  return gtk_tree_drag_source_drag_data_get(gobj(), const_cast<GtkTreePath*>((path).gobj()), (selection_data).gobj());
}

bool TreeDragSource::drag_data_delete(const TreeModel::Path& path)
{
  return gtk_tree_drag_source_drag_data_delete(gobj(), const_cast<GtkTreePath*>((path).gobj()));
}


bool Gtk::TreeDragSource::row_draggable_vfunc(const TreeModel::Path& path) const
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->row_draggable)
    return (*base->row_draggable)(const_cast<GtkTreeDragSource*>(gobj()),const_cast<GtkTreePath*>((path).gobj()));

  typedef bool RType;
  return RType();
}

bool Gtk::TreeDragSource::drag_data_delete_vfunc(const TreeModel::Path& path) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->drag_data_delete)
    return (*base->drag_data_delete)(gobj(),const_cast<GtkTreePath*>((path).gobj()));

  typedef bool RType;
  return RType();
}


} // namespace Gtk


