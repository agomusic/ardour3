// Generated by gtkmmproc -- DO NOT MODIFY!


#include <gtkmm/scrolledwindow.h>
#include <gtkmm/private/scrolledwindow_p.h>

// -*- c++ -*-
/* $Id$ */

/* 
 *
 * Copyright 1998-2002 The gtkmm Development Team
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

#include <gtkmm/scrollbar.h>
#include <gtkmm/viewport.h>
#include <gtkmm/adjustment.h>
#include <gtk/gtkscrolledwindow.h>


namespace Gtk
{

void ScrolledWindow::add(Gtk::Widget& widget)
{
  GtkWidget* gwidget = widget.gobj();

  //This check is courtesy of James Henstridge on gtk-devel-list@gnome.org.
  if( GTK_WIDGET_GET_CLASS(gwidget)->set_scroll_adjustments_signal == 0)
  {
    //It doesn't have native scrolling capability, so it should be put inside a viewport first:
    gtk_scrolled_window_add_with_viewport(gobj(), gwidget);
  }
  else
  {
    //It can work directly with a GtkScrolledWindow, so just use the Container::add():
    Bin::add(widget);
  }
}

} //namespace Gtk


namespace
{
} // anonymous namespace


namespace Glib
{

Gtk::ScrolledWindow* wrap(GtkScrolledWindow* object, bool take_copy)
{
  return dynamic_cast<Gtk::ScrolledWindow *> (Glib::wrap_auto ((GObject*)(object), take_copy));
}

} /* namespace Glib */

namespace Gtk
{


/* The *_Class implementation: */

const Glib::Class& ScrolledWindow_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &ScrolledWindow_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_scrolled_window_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:
  }

  return *this;
}

void ScrolledWindow_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


Glib::ObjectBase* ScrolledWindow_Class::wrap_new(GObject* o)
{
  return manage(new ScrolledWindow((GtkScrolledWindow*)(o)));

}


/* The implementation: */

ScrolledWindow::ScrolledWindow(const Glib::ConstructParams& construct_params)
:
  Gtk::Bin(construct_params)
{
  }

ScrolledWindow::ScrolledWindow(GtkScrolledWindow* castitem)
:
  Gtk::Bin((GtkBin*)(castitem))
{
  }

ScrolledWindow::~ScrolledWindow()
{
  destroy_();
}

ScrolledWindow::CppClassType ScrolledWindow::scrolledwindow_class_; // initialize static member

GType ScrolledWindow::get_type()
{
  return scrolledwindow_class_.init().get_type();
}

GType ScrolledWindow::get_base_type()
{
  return gtk_scrolled_window_get_type();
}


ScrolledWindow::ScrolledWindow()
:
  Glib::ObjectBase(0), //Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
  Gtk::Bin(Glib::ConstructParams(scrolledwindow_class_.init()))
{
  }

ScrolledWindow::ScrolledWindow(Adjustment& hadjustment, Adjustment& vadjustment)
:
  Glib::ObjectBase(0), //Mark this class as gtkmmproc-generated, rather than a custom class, to allow vfunc optimisations.
  Gtk::Bin(Glib::ConstructParams(scrolledwindow_class_.init(), "hadjustment", (hadjustment).gobj(), "vadjustment", (vadjustment).gobj(), (char*) 0))
{
  }

void ScrolledWindow::set_hadjustment(Gtk::Adjustment* hadjustment)
{
gtk_scrolled_window_set_hadjustment(gobj(), (GtkAdjustment*)Glib::unwrap(hadjustment)); 
}

void ScrolledWindow::set_vadjustment(Gtk::Adjustment* vadjustment)
{
gtk_scrolled_window_set_vadjustment(gobj(), (GtkAdjustment*)Glib::unwrap(vadjustment)); 
}

void ScrolledWindow::set_hadjustment(Gtk::Adjustment& hadjustment)
{
gtk_scrolled_window_set_hadjustment(gobj(), (hadjustment).gobj()); 
}

void ScrolledWindow::set_vadjustment(Gtk::Adjustment& vadjustment)
{
gtk_scrolled_window_set_vadjustment(gobj(), (vadjustment).gobj()); 
}

Gtk::Adjustment* ScrolledWindow::get_hadjustment()
{
  return Glib::wrap(gtk_scrolled_window_get_hadjustment(gobj()));
}

const Gtk::Adjustment* ScrolledWindow::get_hadjustment() const
{
  return const_cast<ScrolledWindow*>(this)->get_hadjustment();
}

Gtk::Adjustment* ScrolledWindow::get_vadjustment()
{
  return Glib::wrap(gtk_scrolled_window_get_vadjustment(gobj()));
}

const Gtk::Adjustment* ScrolledWindow::get_vadjustment() const
{
  return const_cast<ScrolledWindow*>(this)->get_vadjustment();
}

void ScrolledWindow::set_policy(PolicyType hscrollbar_policy, PolicyType vscrollbar_policy)
{
gtk_scrolled_window_set_policy(gobj(), ((GtkPolicyType)(hscrollbar_policy)), ((GtkPolicyType)(vscrollbar_policy))); 
}

void ScrolledWindow::get_policy(PolicyType& hscrollbar_policy, PolicyType& vscrollbar_policy) const
{
gtk_scrolled_window_get_policy(const_cast<GtkScrolledWindow*>(gobj()), ((GtkPolicyType*) &(hscrollbar_policy)), ((GtkPolicyType*) &(vscrollbar_policy))); 
}

void ScrolledWindow::set_placement(CornerType window_placement)
{
gtk_scrolled_window_set_placement(gobj(), ((GtkCornerType)(window_placement))); 
}

void ScrolledWindow::unset_placement()
{
gtk_scrolled_window_unset_placement(gobj()); 
}

CornerType ScrolledWindow::get_placement() const
{
  return ((CornerType)(gtk_scrolled_window_get_placement(const_cast<GtkScrolledWindow*>(gobj()))));
}

void ScrolledWindow::set_shadow_type(ShadowType type)
{
gtk_scrolled_window_set_shadow_type(gobj(), ((GtkShadowType)(type))); 
}

ShadowType ScrolledWindow::get_shadow_type() const
{
  return ((ShadowType)(gtk_scrolled_window_get_shadow_type(const_cast<GtkScrolledWindow*>(gobj()))));
}

VScrollbar* ScrolledWindow::get_vscrollbar()
{
  return Glib::wrap((GtkVScrollbar*)gtk_scrolled_window_get_vscrollbar(gobj()));
}

const VScrollbar* ScrolledWindow::get_vscrollbar() const
{
  return Glib::wrap((GtkVScrollbar*)gtk_scrolled_window_get_vscrollbar(const_cast<GtkScrolledWindow*>(gobj())));
}

HScrollbar* ScrolledWindow::get_hscrollbar()
{
  return Glib::wrap((GtkHScrollbar*)gtk_scrolled_window_get_hscrollbar(gobj()));
}

const HScrollbar* ScrolledWindow::get_hscrollbar() const
{
  return Glib::wrap((GtkHScrollbar*)gtk_scrolled_window_get_hscrollbar(const_cast<GtkScrolledWindow*>(gobj())));
}

 bool ScrolledWindow::get_vscrollbar_visible() const
{
  return gobj()->vscrollbar_visible;
}
 
 bool ScrolledWindow::get_hscrollbar_visible() const
{
  return gobj()->hscrollbar_visible;
}
 

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<Gtk::Adjustment*> ScrolledWindow::property_hadjustment() 
{
  return Glib::PropertyProxy<Gtk::Adjustment*>(this, "hadjustment");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<Gtk::Adjustment*> ScrolledWindow::property_hadjustment() const
{
  return Glib::PropertyProxy_ReadOnly<Gtk::Adjustment*>(this, "hadjustment");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<Gtk::Adjustment*> ScrolledWindow::property_vadjustment() 
{
  return Glib::PropertyProxy<Gtk::Adjustment*>(this, "vadjustment");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<Gtk::Adjustment*> ScrolledWindow::property_vadjustment() const
{
  return Glib::PropertyProxy_ReadOnly<Gtk::Adjustment*>(this, "vadjustment");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<PolicyType> ScrolledWindow::property_hscrollbar_policy() 
{
  return Glib::PropertyProxy<PolicyType>(this, "hscrollbar-policy");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<PolicyType> ScrolledWindow::property_hscrollbar_policy() const
{
  return Glib::PropertyProxy_ReadOnly<PolicyType>(this, "hscrollbar-policy");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<PolicyType> ScrolledWindow::property_vscrollbar_policy() 
{
  return Glib::PropertyProxy<PolicyType>(this, "vscrollbar-policy");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<PolicyType> ScrolledWindow::property_vscrollbar_policy() const
{
  return Glib::PropertyProxy_ReadOnly<PolicyType>(this, "vscrollbar-policy");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<CornerType> ScrolledWindow::property_window_placement() 
{
  return Glib::PropertyProxy<CornerType>(this, "window-placement");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<CornerType> ScrolledWindow::property_window_placement() const
{
  return Glib::PropertyProxy_ReadOnly<CornerType>(this, "window-placement");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<ShadowType> ScrolledWindow::property_shadow_type() 
{
  return Glib::PropertyProxy<ShadowType>(this, "shadow-type");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<ShadowType> ScrolledWindow::property_shadow_type() const
{
  return Glib::PropertyProxy_ReadOnly<ShadowType>(this, "shadow-type");
}
#endif //GLIBMM_PROPERTIES_ENABLED


#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED


} // namespace Gtk


