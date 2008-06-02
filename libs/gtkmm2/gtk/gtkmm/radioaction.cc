// Generated by gtkmmproc -- DO NOT MODIFY!


#include <gtkmm/radioaction.h>
#include <gtkmm/private/radioaction_p.h>

// -*- c++ -*-
/* $Id$ */

/* Copyright 2003 The gtkmm Development Team
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

#include <gtk/gtkradioaction.h>


typedef Gtk::RadioAction::Group Group; //So that the generate get_group return type is parsed.

namespace Gtk
{

RadioAction::RadioAction(Group& group, const Glib::ustring& name, const Gtk::StockID& stock_id, const Glib::ustring& label, const Glib::ustring& tooltip)
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::ToggleAction(Glib::ConstructParams(radioaction_class_.init(), "name",name.c_str(),"stock_id",stock_id.get_c_str(),"label",(label.empty() ? 0 : label.c_str()),"tooltip",(tooltip.empty() ? 0 : tooltip.c_str()), static_cast<char*>(0)))
{
  set_group(group);
}

Glib::RefPtr<RadioAction> RadioAction::create(Group& group, const Glib::ustring& name, const Glib::ustring& label, const Glib::ustring& tooltip)
{
  return Glib::RefPtr<RadioAction>( new RadioAction(group, name, Gtk::StockID(), label, tooltip) );
}

Glib::RefPtr<RadioAction> RadioAction::create(Group& group, const Glib::ustring& name, const Gtk::StockID& stock_id, const Glib::ustring& label, const Glib::ustring& tooltip)
{
  return Glib::RefPtr<RadioAction>( new RadioAction(group, name, stock_id, label, tooltip) );
}

void RadioAction::set_group(Group& group)
{
  gtk_radio_action_set_group(gobj(), group.group_);

  //The group will be updated, ready for use with the next radio action:
  group = get_group();
}


} // namespace Gtk


namespace
{


static void RadioAction_signal_changed_callback(GtkRadioAction* self, GtkRadioAction* p0,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,const Glib::RefPtr<RadioAction>& > SlotType;

  // Do not try to call a signal on a disassociated wrapper.
  if(Glib::ObjectBase::_get_current_wrapper((GObject*) self))
  {
    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
    #endif //GLIBMM_EXCEPTIONS_ENABLED
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0, true)
);
    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
    #endif //GLIBMM_EXCEPTIONS_ENABLED
  }
}

static const Glib::SignalProxyInfo RadioAction_signal_changed_info =
{
  "changed",
  (GCallback) &RadioAction_signal_changed_callback,
  (GCallback) &RadioAction_signal_changed_callback
};


} // anonymous namespace


namespace Glib
{

Glib::RefPtr<Gtk::RadioAction> wrap(GtkRadioAction* object, bool take_copy)
{
  return Glib::RefPtr<Gtk::RadioAction>( dynamic_cast<Gtk::RadioAction*> (Glib::wrap_auto ((GObject*)(object), take_copy)) );
  //We use dynamic_cast<> in case of multiple inheritance.
}

} /* namespace Glib */


namespace Gtk
{


/* The *_Class implementation: */

const Glib::Class& RadioAction_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &RadioAction_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_radio_action_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:
  }

  return *this;
}

void RadioAction_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
  klass->changed = &changed_callback;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
void RadioAction_Class::changed_callback(GtkRadioAction* self, GtkRadioAction* p0)
{
  Glib::ObjectBase *const obj_base = static_cast<Glib::ObjectBase*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj_base && obj_base->is_derived_())
  {
    CppObjectType *const obj = dynamic_cast<CppObjectType* const>(obj_base);
    if(obj) // This can be NULL during destruction.
    {
      #ifdef GLIBMM_EXCEPTIONS_ENABLED
      try // Trap C++ exceptions which would normally be lost because this is a C callback.
      {
      #endif //GLIBMM_EXCEPTIONS_ENABLED
        // Call the virtual member method, which derived classes might override.
        obj->on_changed(Glib::wrap(p0, true)
);
        return;
      #ifdef GLIBMM_EXCEPTIONS_ENABLED
      }
      catch(...)
      {
        Glib::exception_handlers_invoke();
      }
      #endif //GLIBMM_EXCEPTIONS_ENABLED
    }
  }
  
  BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_class_peek_parent(G_OBJECT_GET_CLASS(self)) // Get the parent class of the object class (The original underlying C class).
    );

  // Call the original underlying C function:
  if(base && base->changed)
    (*base->changed)(self, p0);
}
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


Glib::ObjectBase* RadioAction_Class::wrap_new(GObject* object)
{
  return new RadioAction((GtkRadioAction*)object);
}


/* The implementation: */

GtkRadioAction* RadioAction::gobj_copy()
{
  reference();
  return gobj();
}

RadioAction::RadioAction(const Glib::ConstructParams& construct_params)
:
  Gtk::ToggleAction(construct_params)
{}

RadioAction::RadioAction(GtkRadioAction* castitem)
:
  Gtk::ToggleAction((GtkToggleAction*)(castitem))
{}

RadioAction::~RadioAction()
{}


RadioAction::CppClassType RadioAction::radioaction_class_; // initialize static member

GType RadioAction::get_type()
{
  return radioaction_class_.init().get_type();
}

GType RadioAction::get_base_type()
{
  return gtk_radio_action_get_type();
}


RadioAction::RadioAction()
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::ToggleAction(Glib::ConstructParams(radioaction_class_.init()))
{
  }

Glib::RefPtr<RadioAction> RadioAction::create()
{
  return Glib::RefPtr<RadioAction>( new RadioAction() );
}
Group RadioAction::get_group()
{
  return Group(gtk_radio_action_get_group(gobj()));
}

int RadioAction::get_current_value() const
{
  return gtk_radio_action_get_current_value(const_cast<GtkRadioAction*>(gobj()));
}

void RadioAction::set_current_value(int current_value)
{
gtk_radio_action_set_current_value(gobj(), current_value); 
}


Glib::SignalProxy1< void,const Glib::RefPtr<RadioAction>& > RadioAction::signal_changed()
{
  return Glib::SignalProxy1< void,const Glib::RefPtr<RadioAction>& >(this, &RadioAction_signal_changed_info);
}


#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> RadioAction::property_value() 
{
  return Glib::PropertyProxy<int>(this, "value");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> RadioAction::property_value() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "value");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> RadioAction::property_current_value() 
{
  return Glib::PropertyProxy<int>(this, "current-value");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> RadioAction::property_current_value() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "current-value");
}
#endif //GLIBMM_PROPERTIES_ENABLED


#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
void Gtk::RadioAction::on_changed(const Glib::RefPtr<RadioAction>& current)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->changed)
    (*base->changed)(gobj(),Glib::unwrap(current));
}
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED


} // namespace Gtk


