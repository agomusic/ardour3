// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_BUILDER_H
#define _GTKMM_BUILDER_H


#include <glibmm.h>

/* $Id: builder.hg,v 1.19 2006/05/10 20:59:27 murrayc Exp $ */

/* Copyright (C) 2007 The gtkmm Development Team
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


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkBuilder GtkBuilder;
typedef struct _GtkBuilderClass GtkBuilderClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class Builder_Class; } // namespace Gtk
namespace Gtk
{

/** Exception class for Gdk::Builder errors.
 */
class BuilderError : public Glib::Error
{
public:
  enum Code
  {
    INVALID_TYPE_FUNCTION,
    UNHANDLED_TAG,
    MISSING_ATTRIBUTE,
    INVALID_ATTRIBUTE,
    INVALID_TAG,
    MISSING_PROPERTY_VALUE,
    INVALID_VALUE
  };

  BuilderError(Code error_code, const Glib::ustring& error_message);
  explicit BuilderError(GError* gobject);
  Code code() const;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
private:

#ifdef GLIBMM_EXCEPTIONS_ENABLED
  static void throw_func(GError* gobject);
#else
  //When not using exceptions, we just pass the Exception object around without throwing it:
  static std::auto_ptr<Glib::Error> throw_func(GError* gobject);
#endif //GLIBMM_EXCEPTIONS_ENABLED

  friend void wrap_init(); // uses throw_func()
#endif
};

} // namespace Gtk

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gtk::BuilderError::Code> : public Glib::Value_Enum<Gtk::BuilderError::Code>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{


/** Build an interface from a UI definition description.
 *
 * This object represents an `instantiation' of an UI definition description.
 * When one of these objects is created, the XML file is read, and the interface 
 * is created. The Gtk::Builder object then provides an interface for accessing the 
 * widgets in the interface by the names assigned to them inside the UI description.
 *
 * @newin2p12
 */

class Builder : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef Builder CppObjectType;
  typedef Builder_Class CppClassType;
  typedef GtkBuilder BaseObjectType;
  typedef GtkBuilderClass BaseClassType;

private:  friend class Builder_Class;
  static CppClassType builder_class_;

private:
  // noncopyable
  Builder(const Builder&);
  Builder& operator=(const Builder&);

protected:
  explicit Builder(const Glib::ConstructParams& construct_params);
  explicit Builder(GtkBuilder* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~Builder();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GtkBuilder*       gobj()       { return reinterpret_cast<GtkBuilder*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GtkBuilder* gobj() const { return reinterpret_cast<GtkBuilder*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GtkBuilder* gobj_copy();

private:

 
protected:
 Builder();
 

public:
  /** Creates a new builder object. 
   */
  
  static Glib::RefPtr<Builder> create();


  /** Parses a file containing a GtkBuilder UI definition.
   *
   * @param filename the name of the file to parse.
   * @result A new Builder object, or a null pointer if an error occurred.
   * @throws Glib::FileError, Glib::MarkupError
   *
   * @newin2p12
   **/
  static Glib::RefPtr<Builder> create_from_file(const std::string& filename);


  /** Parses a string containing a GtkBuilder UI definition.
   *
   * @buffer: the string to parse
   * @result A new Builder object, or a null pointer if an error occurred.
   * @throws Glib::MarkupError
   *
   * @newin2p12
   **/
  static Glib::RefPtr<Builder> create_from_string(const Glib::ustring& buffer);

  
  /** Parses a file containing a GtkBuilder 
   * UI definition and merges it with the current contents of @a builder .
   * @param filename The name of the file to parse.
   * @param error Return location for an error, or <tt>0</tt>.
   * @return A positive value on success, 0 if an error occurred
   * 
   * @newin2p12.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  bool add_from_file(const std::string& filename);
#else
  bool add_from_file(const std::string& filename, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED


  /** Parses a string containing a GtkBuilder UI definition 
   * and merges it with the current contents of the builder.
   *
   * @buffer: the string to parse
   * @result true on success or false if an error occurred.
   * @throws Glib::MarkupError
   *
   * @newin2p12
   **/
  #ifdef GLIBMM_EXCEPTIONS_ENABLED
  bool add_from_string(const Glib::ustring& buffer);
  #else
  bool add_from_string(const Glib::ustring& buffer, std::auto_ptr<Glib::Error>& error);
  #endif //GLIBMM_EXCEPTIONS_ENABLED

  
  /** Parses a string containing a GtkBuilder 
   * UI definition and merges it with the current contents of @a builder .
   * @param buffer The string to parse.
   * @param length The length of @a buffer  (may be -1 if @a buffer  is nul-terminated).
   * @param error Return location for an error, or <tt>0</tt>.
   * @return A positive value on success, 0 if an error occurred
   * 
   * @newin2p12.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  bool add_from_string(const char* buffer, gsize length);
#else
  bool add_from_string(const char* buffer, gsize length, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED


  //TODO: Custom-implement this and prevent it from being used with GTK_WIDGET-derived types?
  
  /** Gets the object named @a name . Note that this function does not
   * increment the reference count of the returned object.
   * @param name Name of object to get.
   * @return The object named @a name  or <tt>0</tt> if it could not be 
   * found in the object tree. 
   * 
   * @newin2p12.
   */
  Glib::RefPtr<Glib::Object> get_object(const Glib::ustring& name);

  /** Get a widget from the Builder file.
   * For instance:
   * @code
   * Gtk::Table* pTable = 0;
   * refXml->get_widget("mytable", pTable);
   * @endcode
   * This method prints a warning message to the console if the widget
   * doesn't exist or has the wrong type, so you don't need to check that
   * manually.
   *
   * Note that you are responsible for deleting top-level widgets (windows and
   * dialogs) instantiated by the Builder object. Other widgets are instantiated as
   * managed so they will be deleted automatically if you add them to a
   * container widget.
   *
   * @param name The name of the widget.
   * @retval widget A pointer to the widget, or <tt>0</tt> on failure.
   */
  template <class T_Widget> inline
  void get_widget(const Glib::ustring& name, T_Widget*& widget)
  {
    // Initialize output parameter:
    widget = 0;

    // The dynamic_cast<> should never fail if get_widget_checked() succeeded.
    widget = dynamic_cast<T_Widget*>(this->get_widget_checked(name, T_Widget::get_base_type()));

    if(!widget)
      g_critical("Gtk::Builder::get_widget(): dynamic_cast<> failed.");
  }

  /** This provides a pointer to a widget whose details are specified in the GtkBuilder file, but which is implemented
   * by your own derived class. Your class must have a constructor like so:
   * @code
   * DerivedDialog::DerivedDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder)
   * : Gtk::Dialog(cobject) //Calls the base class constructor
   * @endcode
   *
   * For instance:
   * @code
   * Gtk::DerivedBox* pBox = 0;
   * refXml->get_widget_derived("mybox", pBox);
   * @endcode
   *
   * @param name The name of the widget.
   * @retval widget A pointer to the widget, or <tt>0</tt> on failure.
   */
  template <class T_Widget> inline
  void get_widget_derived(const Glib::ustring& name, T_Widget*& widget)
  {
     // Initialize output parameter:
     widget = 0;
     
     // Get the widget from the glade file.
     typedef typename T_Widget::BaseObjectType cwidget_type;
     cwidget_type* pCWidget = (cwidget_type*)get_cwidget(name);

     //The error was already reported by get_cwidget().
     if(!pCWidget)
       return;

     //Check whether there is already a C++ wrapper instance associated with this C instance:
     Glib::ObjectBase* pObjectBase = ObjectBase::_get_current_wrapper((GObject*)pCWidget);

     //If there is already a C++ instance, then return it again:
     if(pObjectBase)
     {
       widget = dynamic_cast<T_Widget*>( Glib::wrap((GtkWidget*)pCWidget) );
       //Newer, more spec-complaint, versions of g++ cannot resolve a specific wrap() function in a template.

       //The dynamic cast checks that it is of the correct type.
       //Somebody might be trying to call get_widget_derived() after already calling get_widget(),
       //or after already calling get_widget_derived() with a different derived C++ type.
       if(!widget)
        g_critical("Gtk::Builder::get_widget_derived(): dynamic_cast<> failed. An existing C++ instance, of a different type, seems to exist.");      
     }
     else
     {
       //Create a new C++ instance to wrap the existing C instance:
       
       //Set the output variable. We needed to do this because we can not template the return type.
       Glib::RefPtr<Gtk::Builder> refThis(this);
       refThis->reference(); //take a copy.
       widget = new T_Widget(pCWidget, refThis);
     }
  }


  //TODO:_WRAP_METHOD(GSList* get_objects(), gtk_builder_get_objects)
  //_WRAP_METHOD(void connect_signals(gpointer       user_data), gtk_builder_connect_signals)
  //_WRAP_METHOD(void connect_signals_full(GtkBuilderConnectFunc func, gpointer       user_data), gtk_builder_connect_signals_full)
  
  /** Sets the translation domain of @a builder . 
   * See Gtk::Builder:translation-domain.
   * 
   * @newin2p12
   * @param domain The translation domain or <tt>0</tt>.
   */
  void set_translation_domain(const Glib::ustring& domain);
  
  /** Gets the translation domain of @a builder .
   * @return The translation domain. This string is owned
   * by the builder object and must not be modified or freed.
   * 
   * @newin2p12.
   */
  Glib::ustring get_translation_domain() const;
  
  //We ignore gtk_builder_get_type_from_name() because it only seems useful when implementing GtkBuildable for widgets.
  

protected:
  Gtk::Widget* get_widget_checked(const Glib::ustring& name, GType type);
  GtkWidget* get_cwidget(const Glib::ustring& name);


  // TODO: Make sure this is actually invoked even with disabled vfuncs so
  // that glade creates gtkmm's GObjects.


public:

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
   * @relates Gtk::Builder
   */
  Glib::RefPtr<Gtk::Builder> wrap(GtkBuilder* object, bool take_copy = false);
}


#endif /* _GTKMM_BUILDER_H */

