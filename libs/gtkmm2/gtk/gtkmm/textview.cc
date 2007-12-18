// Generated by gtkmmproc -- DO NOT MODIFY!


#include <gtkmm/textview.h>
#include <gtkmm/private/textview_p.h>

#include <gtk/gtktypebuiltins.h>
// -*- c++ -*-
/* $Id$ */

/* Copyright 2002 The gtkmm Development Team
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

#include <gtk/gtktextview.h>

namespace Gtk
{

TextView::TextView(const Glib::RefPtr<TextBuffer>& buffer)
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::Container(Glib::ConstructParams(textview_class_.init()))
{
  set_buffer(buffer);
}

bool TextView::scroll_to(TextBuffer::iterator& iter, double within_margin)
{
  //The last 2 arguments are ignored if use_align is FALSE.
  return gtk_text_view_scroll_to_iter(gobj(), (iter).gobj(), within_margin, FALSE, 0.0, 0.0);
}

bool TextView::scroll_to(TextBuffer::iterator& iter, double within_margin, double xalign, double yalign)
{
  return gtk_text_view_scroll_to_iter(gobj(), (iter).gobj(), within_margin, TRUE /* use_align */, xalign, yalign);
}

void TextView::scroll_to(const Glib::RefPtr<TextBuffer::Mark>& mark, double within_margin)
{
  //The last 2 arguments are ignored if use_align is FALSE.
  gtk_text_view_scroll_to_mark(gobj(), Glib::unwrap(mark), within_margin, FALSE, 0.0, 0.0);
}

void TextView::scroll_to(const Glib::RefPtr<TextBuffer::Mark>& mark, double within_margin, double xalign, double yalign)
{
  gtk_text_view_scroll_to_mark(gobj(), Glib::unwrap(mark), within_margin, TRUE /* use_align */, xalign, yalign);
}

#ifndef GTKMM_DISABLE_DEPRECATED

bool TextView::scroll_to_iter(TextBuffer::iterator& iter, double within_margin)
{
  return scroll_to(iter, within_margin);
}

void TextView::scroll_to_mark(const Glib::RefPtr<TextBuffer::Mark>& mark, double within_margin)
{
  scroll_to(mark, within_margin);
}
  
void TextView::scroll_mark_onscreen(const Glib::RefPtr<TextBuffer::Mark>& mark)
{
  scroll_to(mark);
}
#endif // GTKMM_DISABLE_DEPRECATED


} // namespace Gtk


namespace
{


static void TextView_signal_set_scroll_adjustments_callback(GtkTextView* self, GtkAdjustment* p0,GtkAdjustment* p1,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,Adjustment*,Adjustment* > SlotType;

  // Do not try to call a signal on a disassociated wrapper.
  if(Glib::ObjectBase::_get_current_wrapper((GObject*) self))
  {
    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
    #endif //GLIBMM_EXCEPTIONS_ENABLED
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0)
, Glib::wrap(p1)
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

static const Glib::SignalProxyInfo TextView_signal_set_scroll_adjustments_info =
{
  "set_scroll_adjustments",
  (GCallback) &TextView_signal_set_scroll_adjustments_callback,
  (GCallback) &TextView_signal_set_scroll_adjustments_callback
};


static void TextView_signal_populate_popup_callback(GtkTextView* self, GtkMenu* p0,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,Menu* > SlotType;

  // Do not try to call a signal on a disassociated wrapper.
  if(Glib::ObjectBase::_get_current_wrapper((GObject*) self))
  {
    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
    #endif //GLIBMM_EXCEPTIONS_ENABLED
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0)
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

static const Glib::SignalProxyInfo TextView_signal_populate_popup_info =
{
  "populate_popup",
  (GCallback) &TextView_signal_populate_popup_callback,
  (GCallback) &TextView_signal_populate_popup_callback
};


static const Glib::SignalProxyInfo TextView_signal_set_anchor_info =
{
  "set_anchor",
  (GCallback) &Glib::SignalProxyNormal::slot0_void_callback,
  (GCallback) &Glib::SignalProxyNormal::slot0_void_callback
};


static void TextView_signal_insert_at_cursor_callback(GtkTextView* self, const gchar* p0,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,const Glib::ustring& > SlotType;

  // Do not try to call a signal on a disassociated wrapper.
  if(Glib::ObjectBase::_get_current_wrapper((GObject*) self))
  {
    #ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
    #endif //GLIBMM_EXCEPTIONS_ENABLED
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::convert_const_gchar_ptr_to_ustring(p0)
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

static const Glib::SignalProxyInfo TextView_signal_insert_at_cursor_info =
{
  "insert_at_cursor",
  (GCallback) &TextView_signal_insert_at_cursor_callback,
  (GCallback) &TextView_signal_insert_at_cursor_callback
};


} // anonymous namespace

// static
GType Glib::Value<Gtk::TextWindowType>::value_type()
{
  return gtk_text_window_type_get_type();
}


namespace Glib
{

Gtk::TextView* wrap(GtkTextView* object, bool take_copy)
{
  return dynamic_cast<Gtk::TextView *> (Glib::wrap_auto ((GObject*)(object), take_copy));
}

} /* namespace Glib */

namespace Gtk
{


/* The *_Class implementation: */

const Glib::Class& TextView_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &TextView_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_text_view_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:
  }

  return *this;
}

void TextView_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
  klass->set_scroll_adjustments = &set_scroll_adjustments_callback;
  klass->populate_popup = &populate_popup_callback;
  klass->set_anchor = &set_anchor_callback;
  klass->insert_at_cursor = &insert_at_cursor_callback;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
void TextView_Class::set_scroll_adjustments_callback(GtkTextView* self, GtkAdjustment* p0, GtkAdjustment* p1)
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
        obj->on_set_scroll_adjustments(Glib::wrap(p0)
, Glib::wrap(p1)
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
  if(base && base->set_scroll_adjustments)
    (*base->set_scroll_adjustments)(self, p0, p1);
}
void TextView_Class::populate_popup_callback(GtkTextView* self, GtkMenu* p0)
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
        obj->on_populate_popup(Glib::wrap(p0)
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
  if(base && base->populate_popup)
    (*base->populate_popup)(self, p0);
}
void TextView_Class::set_anchor_callback(GtkTextView* self)
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
        obj->on_set_anchor();
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
  if(base && base->set_anchor)
    (*base->set_anchor)(self);
}
void TextView_Class::insert_at_cursor_callback(GtkTextView* self, const gchar* p0)
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
        obj->on_insert_at_cursor(Glib::convert_const_gchar_ptr_to_ustring(p0)
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
  if(base && base->insert_at_cursor)
    (*base->insert_at_cursor)(self, p0);
}
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


Glib::ObjectBase* TextView_Class::wrap_new(GObject* o)
{
  return manage(new TextView((GtkTextView*)(o)));

}


/* The implementation: */

TextView::TextView(const Glib::ConstructParams& construct_params)
:
  Gtk::Container(construct_params)
{
  }

TextView::TextView(GtkTextView* castitem)
:
  Gtk::Container((GtkContainer*)(castitem))
{
  }

TextView::~TextView()
{
  destroy_();
}

TextView::CppClassType TextView::textview_class_; // initialize static member

GType TextView::get_type()
{
  return textview_class_.init().get_type();
}

GType TextView::get_base_type()
{
  return gtk_text_view_get_type();
}


TextView::TextView()
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::Container(Glib::ConstructParams(textview_class_.init()))
{
  }

void TextView::set_buffer(const Glib::RefPtr<TextBuffer>& buffer)
{
gtk_text_view_set_buffer(gobj(), Glib::unwrap(buffer)); 
}

Glib::RefPtr<TextBuffer> TextView::get_buffer()
{

  Glib::RefPtr<TextBuffer> retvalue = Glib::wrap(gtk_text_view_get_buffer(gobj()));
  if(retvalue)
    retvalue->reference(); //The function does not do a ref for us.
  return retvalue;

}

Glib::RefPtr<const TextBuffer> TextView::get_buffer() const
{
  return const_cast<TextView*>(this)->get_buffer();
}

bool TextView::move_mark_onscreen(const Glib::RefPtr<TextBuffer::Mark>& mark)
{
  return gtk_text_view_move_mark_onscreen(gobj(), Glib::unwrap(mark));
}

bool TextView::place_cursor_onscreen()
{
  return gtk_text_view_place_cursor_onscreen(gobj());
}

void TextView::get_visible_rect(Gdk::Rectangle& visible_rect) const
{
gtk_text_view_get_visible_rect(const_cast<GtkTextView*>(gobj()), (visible_rect).gobj()); 
}

void TextView::set_cursor_visible(bool setting)
{
gtk_text_view_set_cursor_visible(gobj(), static_cast<int>(setting)); 
}

bool TextView::get_cursor_visible() const
{
  return gtk_text_view_get_cursor_visible(const_cast<GtkTextView*>(gobj()));
}

void TextView::get_iter_location(const TextBuffer::iterator& iter, Gdk::Rectangle& location) const
{
gtk_text_view_get_iter_location(const_cast<GtkTextView*>(gobj()), (iter).gobj(), (location).gobj()); 
}

void TextView::get_iter_at_location(TextBuffer::iterator& iter, int x, int y) const
{
gtk_text_view_get_iter_at_location(const_cast<GtkTextView*>(gobj()), (iter).gobj(), x, y); 
}

void TextView::get_iter_at_position(TextBuffer::iterator& iter, int& trailing, int x, int y) const
{
gtk_text_view_get_iter_at_position(const_cast<GtkTextView*>(gobj()), (iter).gobj(), &trailing, x, y); 
}

void TextView::get_line_yrange(const TextBuffer::iterator& iter, int& y, int& height) const
{
gtk_text_view_get_line_yrange(const_cast<GtkTextView*>(gobj()), (iter).gobj(), &y, &height); 
}

void TextView::get_line_at_y(TextBuffer::iterator& target_iter, int y, int& line_top) const
{
gtk_text_view_get_line_at_y(const_cast<GtkTextView*>(gobj()), (target_iter).gobj(), y, &line_top); 
}

void TextView::buffer_to_window_coords(TextWindowType win, int buffer_x, int buffer_y, int& window_x, int& window_y) const
{
gtk_text_view_buffer_to_window_coords(const_cast<GtkTextView*>(gobj()), ((GtkTextWindowType)(win)), buffer_x, buffer_y, &window_x, &window_y); 
}

void TextView::window_to_buffer_coords(TextWindowType win, int window_x, int window_y, int& buffer_x, int& buffer_y) const
{
gtk_text_view_window_to_buffer_coords(const_cast<GtkTextView*>(gobj()), ((GtkTextWindowType)(win)), window_x, window_y, &buffer_x, &buffer_y); 
}

Glib::RefPtr<Gdk::Window> TextView::get_window(TextWindowType win)
{

  Glib::RefPtr<Gdk::Window> retvalue = Glib::wrap((GdkWindowObject*)(gtk_text_view_get_window(gobj(), ((GtkTextWindowType)(win)))));
  if(retvalue)
    retvalue->reference(); //The function does not do a ref for us.
  return retvalue;

}

Glib::RefPtr<const Gdk::Window> TextView::get_window(TextWindowType win) const
{
  return const_cast<TextView*>(this)->get_window(win);
}

TextWindowType TextView::get_window_type(const Glib::RefPtr<Gdk::Window>& window)
{
  return ((TextWindowType)(gtk_text_view_get_window_type(gobj(), Glib::unwrap(window))));
}

void TextView::set_border_window_size(TextWindowType type, int size)
{
gtk_text_view_set_border_window_size(gobj(), ((GtkTextWindowType)(type)), size); 
}

int TextView::get_border_window_size(TextWindowType type) const
{
  return gtk_text_view_get_border_window_size(const_cast<GtkTextView*>(gobj()), ((GtkTextWindowType)(type)));
}

bool TextView::forward_display_line(TextBuffer::iterator& iter)
{
  return gtk_text_view_forward_display_line(gobj(), (iter).gobj());
}

bool TextView::backward_display_line(TextBuffer::iterator& iter)
{
  return gtk_text_view_backward_display_line(gobj(), (iter).gobj());
}

bool TextView::forward_display_line_end(TextBuffer::iterator& iter)
{
  return gtk_text_view_forward_display_line_end(gobj(), (iter).gobj());
}

bool TextView::backward_display_line_start(TextBuffer::iterator& iter)
{
  return gtk_text_view_backward_display_line_start(gobj(), (iter).gobj());
}

bool TextView::starts_display_line(const TextBuffer::iterator& iter)
{
  return gtk_text_view_starts_display_line(gobj(), (iter).gobj());
}

bool TextView::move_visually(TextBuffer::iterator& iter, int count)
{
  return gtk_text_view_move_visually(gobj(), (iter).gobj(), count);
}

void TextView::add_child_at_anchor(Widget& child, const Glib::RefPtr<TextBuffer::ChildAnchor>& anchor)
{
gtk_text_view_add_child_at_anchor(gobj(), (child).gobj(), Glib::unwrap(anchor)); 
}

void TextView::add_child_in_window(Widget& child, TextWindowType which_window, int xpos, int ypos)
{
gtk_text_view_add_child_in_window(gobj(), (child).gobj(), ((GtkTextWindowType)(which_window)), xpos, ypos); 
}

void TextView::move_child(Widget& child, int xpos, int ypos)
{
gtk_text_view_move_child(gobj(), (child).gobj(), xpos, ypos); 
}

void TextView::set_wrap_mode(WrapMode wrap_mode)
{
gtk_text_view_set_wrap_mode(gobj(), ((GtkWrapMode)(wrap_mode))); 
}

WrapMode TextView::get_wrap_mode() const
{
  return ((WrapMode)(gtk_text_view_get_wrap_mode(const_cast<GtkTextView*>(gobj()))));
}

void TextView::set_editable(bool setting)
{
gtk_text_view_set_editable(gobj(), static_cast<int>(setting)); 
}

bool TextView::get_editable() const
{
  return gtk_text_view_get_editable(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_pixels_above_lines(int pixels_above_lines)
{
gtk_text_view_set_pixels_above_lines(gobj(), pixels_above_lines); 
}

int TextView::get_pixels_above_lines() const
{
  return gtk_text_view_get_pixels_above_lines(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_pixels_below_lines(int pixels_below_lines)
{
gtk_text_view_set_pixels_below_lines(gobj(), pixels_below_lines); 
}

int TextView::get_pixels_below_lines() const
{
  return gtk_text_view_get_pixels_below_lines(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_pixels_inside_wrap(int pixels_inside_wrap)
{
gtk_text_view_set_pixels_inside_wrap(gobj(), pixels_inside_wrap); 
}

int TextView::get_pixels_inside_wrap() const
{
  return gtk_text_view_get_pixels_inside_wrap(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_justification(Justification justification)
{
gtk_text_view_set_justification(gobj(), ((GtkJustification)(justification))); 
}

Justification TextView::get_justification() const
{
  return ((Justification)(gtk_text_view_get_justification(const_cast<GtkTextView*>(gobj()))));
}

void TextView::set_left_margin(int left_margin)
{
gtk_text_view_set_left_margin(gobj(), left_margin); 
}

int TextView::get_left_margin() const
{
  return gtk_text_view_get_left_margin(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_right_margin(int right_margin)
{
gtk_text_view_set_right_margin(gobj(), right_margin); 
}

int TextView::get_right_margin() const
{
  return gtk_text_view_get_right_margin(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_indent(int indent)
{
gtk_text_view_set_indent(gobj(), indent); 
}

int TextView::get_indent() const
{
  return gtk_text_view_get_indent(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_tabs(Pango::TabArray& tabs)
{
gtk_text_view_set_tabs(gobj(), (tabs).gobj()); 
}

Pango::TabArray TextView::get_tabs() const
{
  return Pango::TabArray((gtk_text_view_get_tabs(const_cast<GtkTextView*>(gobj()))));
}

TextAttributes TextView::get_default_attributes() const
{
  return TextAttributes(gtk_text_view_get_default_attributes(const_cast<GtkTextView*>(gobj())));
}

void TextView::set_overwrite(bool overwrite)
{
gtk_text_view_set_overwrite(gobj(), static_cast<int>(overwrite)); 
}

bool TextView::get_overwrite() const
{
  return gtk_text_view_get_overwrite(const_cast<GtkTextView*>(gobj()));
}

void TextView::set_accepts_tab(bool accepts_tab)
{
gtk_text_view_set_accepts_tab(gobj(), static_cast<int>(accepts_tab)); 
}

bool TextView::get_accepts_tab() const
{
  return gtk_text_view_get_accepts_tab(const_cast<GtkTextView*>(gobj()));
}


Glib::SignalProxy2< void,Adjustment*,Adjustment* > TextView::signal_set_scroll_adjustments()
{
  return Glib::SignalProxy2< void,Adjustment*,Adjustment* >(this, &TextView_signal_set_scroll_adjustments_info);
}


Glib::SignalProxy1< void,Menu* > TextView::signal_populate_popup()
{
  return Glib::SignalProxy1< void,Menu* >(this, &TextView_signal_populate_popup_info);
}


Glib::SignalProxy0< void > TextView::signal_set_anchor()
{
  return Glib::SignalProxy0< void >(this, &TextView_signal_set_anchor_info);
}


Glib::SignalProxy1< void,const Glib::ustring& > TextView::signal_insert_at_cursor()
{
  return Glib::SignalProxy1< void,const Glib::ustring& >(this, &TextView_signal_insert_at_cursor_info);
}


#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_pixels_above_lines() 
{
  return Glib::PropertyProxy<int>(this, "pixels-above-lines");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_pixels_above_lines() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "pixels-above-lines");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_pixels_below_lines() 
{
  return Glib::PropertyProxy<int>(this, "pixels-below-lines");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_pixels_below_lines() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "pixels-below-lines");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_pixels_inside_wrap() 
{
  return Glib::PropertyProxy<int>(this, "pixels-inside-wrap");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_pixels_inside_wrap() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "pixels-inside-wrap");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<bool> TextView::property_editable() 
{
  return Glib::PropertyProxy<bool>(this, "editable");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<bool> TextView::property_editable() const
{
  return Glib::PropertyProxy_ReadOnly<bool>(this, "editable");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<WrapMode> TextView::property_wrap_mode() 
{
  return Glib::PropertyProxy<WrapMode>(this, "wrap-mode");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<WrapMode> TextView::property_wrap_mode() const
{
  return Glib::PropertyProxy_ReadOnly<WrapMode>(this, "wrap-mode");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<Justification> TextView::property_justification() 
{
  return Glib::PropertyProxy<Justification>(this, "justification");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<Justification> TextView::property_justification() const
{
  return Glib::PropertyProxy_ReadOnly<Justification>(this, "justification");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_left_margin() 
{
  return Glib::PropertyProxy<int>(this, "left-margin");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_left_margin() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "left-margin");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_right_margin() 
{
  return Glib::PropertyProxy<int>(this, "right-margin");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_right_margin() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "right-margin");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<int> TextView::property_indent() 
{
  return Glib::PropertyProxy<int>(this, "indent");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<int> TextView::property_indent() const
{
  return Glib::PropertyProxy_ReadOnly<int>(this, "indent");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<Pango::TabArray> TextView::property_tabs() 
{
  return Glib::PropertyProxy<Pango::TabArray>(this, "tabs");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<Pango::TabArray> TextView::property_tabs() const
{
  return Glib::PropertyProxy_ReadOnly<Pango::TabArray>(this, "tabs");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<bool> TextView::property_cursor_visible() 
{
  return Glib::PropertyProxy<bool>(this, "cursor-visible");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<bool> TextView::property_cursor_visible() const
{
  return Glib::PropertyProxy_ReadOnly<bool>(this, "cursor-visible");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy< Glib::RefPtr<TextBuffer> > TextView::property_buffer() 
{
  return Glib::PropertyProxy< Glib::RefPtr<TextBuffer> >(this, "buffer");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly< Glib::RefPtr<TextBuffer> > TextView::property_buffer() const
{
  return Glib::PropertyProxy_ReadOnly< Glib::RefPtr<TextBuffer> >(this, "buffer");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<bool> TextView::property_overwrite() 
{
  return Glib::PropertyProxy<bool>(this, "overwrite");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<bool> TextView::property_overwrite() const
{
  return Glib::PropertyProxy_ReadOnly<bool>(this, "overwrite");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy<bool> TextView::property_accepts_tab() 
{
  return Glib::PropertyProxy<bool>(this, "accepts-tab");
}
#endif //GLIBMM_PROPERTIES_ENABLED

#ifdef GLIBMM_PROPERTIES_ENABLED
Glib::PropertyProxy_ReadOnly<bool> TextView::property_accepts_tab() const
{
  return Glib::PropertyProxy_ReadOnly<bool>(this, "accepts-tab");
}
#endif //GLIBMM_PROPERTIES_ENABLED


#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
void Gtk::TextView::on_set_scroll_adjustments(Adjustment* hadjustment, Adjustment* vadjustment)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->set_scroll_adjustments)
    (*base->set_scroll_adjustments)(gobj(),(GtkAdjustment*)Glib::unwrap(hadjustment),(GtkAdjustment*)Glib::unwrap(vadjustment));
}
void Gtk::TextView::on_populate_popup(Menu* menu)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->populate_popup)
    (*base->populate_popup)(gobj(),(GtkMenu*)Glib::unwrap(menu));
}
void Gtk::TextView::on_set_anchor()
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->set_anchor)
    (*base->set_anchor)(gobj());
}
void Gtk::TextView::on_insert_at_cursor(const Glib::ustring& str)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->insert_at_cursor)
    (*base->insert_at_cursor)(gobj(),str.c_str());
}
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED

#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED


} // namespace Gtk


