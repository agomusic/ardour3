// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_RECENTCHOOSERMENU_H
#define _GTKMM_RECENTCHOOSERMENU_H


#include <glibmm.h>

/* recentchoosermenu.h
 *
 * Copyright (C) 2006 The gtkmm Development Team
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

#include <gtkmm/menu.h>
#include <gtkmm/recentchooser.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkRecentChooserMenu GtkRecentChooserMenu;
typedef struct _GtkRecentChooserMenuClass GtkRecentChooserMenuClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class RecentChooserMenu_Class; } // namespace Gtk
namespace Gtk
{

/** RecentChooserMenu is a widget suitable for displaying recently used files
 * inside a menu.  It can be used to set a sub-menu of a MenuItem using
 * MenuItem::item_set_submenu(), or as the menu of a MenuToolButton.
 *
 * Note that RecentChooserMenu does not have any methods of its own. Instead,
 * you should use the functions that work on a RecentChooser.
 *
 * @newin2p10
 *
 * @ingroup RecentFiles
 */

class RecentChooserMenu
  : public Menu,
    public RecentChooser
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef RecentChooserMenu CppObjectType;
  typedef RecentChooserMenu_Class CppClassType;
  typedef GtkRecentChooserMenu BaseObjectType;
  typedef GtkRecentChooserMenuClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~RecentChooserMenu();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class RecentChooserMenu_Class;
  static CppClassType recentchoosermenu_class_;

  // noncopyable
  RecentChooserMenu(const RecentChooserMenu&);
  RecentChooserMenu& operator=(const RecentChooserMenu&);

protected:
  explicit RecentChooserMenu(const Glib::ConstructParams& construct_params);
  explicit RecentChooserMenu(GtkRecentChooserMenu* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkRecentChooserMenu*       gobj()       { return reinterpret_cast<GtkRecentChooserMenu*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkRecentChooserMenu* gobj() const { return reinterpret_cast<GtkRecentChooserMenu*>(gobject_); }


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
  RecentChooserMenu();

  explicit RecentChooserMenu(const Glib::RefPtr<RecentManager>& recent_manager);

  
  /** Sets whether a number should be added to the items of @a menu .  The
   * numbers are shown to provide a unique character for a mnemonic to
   * be used inside ten menu item's label.  Only the first the items
   * get a number to avoid clashes.
   * 
   * @newin2p10
   * @param show_numbers Whether to show numbers.
   */
  void set_show_numbers(bool show_numbers = true);
  
  /** Return value: <tt>true</tt> if numbers should be shown.
   * @return <tt>true</tt> if numbers should be shown.
   * 
   * @newin2p10.
   */
  bool get_show_numbers() const;


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
   * @relates Gtk::RecentChooserMenu
   */
  Gtk::RecentChooserMenu* wrap(GtkRecentChooserMenu* object, bool take_copy = false);
} //namespace Glib


#endif /* _GTKMM_RECENTCHOOSERMENU_H */

