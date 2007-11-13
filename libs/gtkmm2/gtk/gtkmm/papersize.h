// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_PAPERSIZE_H
#define _GTKMM_PAPERSIZE_H


#include <glibmm.h>

/* Copyright (C) 2006 The gtkmm Development Team
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


#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _GtkPaperSize GtkPaperSize; }
#endif

namespace Gtk
{

/** Common paper names, from PWG 5101.1-2002 PWG: Standard for Media Standardized Names
 *
 */
const Glib::ustring PAPER_NAME_A3 = "iso_a3";
const Glib::ustring PAPER_NAME_A4 = "iso_a4";
const Glib::ustring PAPER_NAME_A5 = "iso_a5";
const Glib::ustring PAPER_NAME_B5 = "iso_b5";
const Glib::ustring PAPER_NAME_LETTER = "na_letter";
const Glib::ustring PAPER_NAME_EXECUTIVE = "na_executive";
const Glib::ustring PAPER_NAME_LEGAL = "na_legal";

/** @addtogroup gtkmmEnums Enums and Flags */

/**
 * @ingroup gtkmmEnums
 */
enum Unit
{
  UNIT_PIXEL,
  UNIT_POINTS,
  UNIT_INCH,
  UNIT_MM
};

} // namespace Gtk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gtk::Unit> : public Glib::Value_Enum<Gtk::Unit>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{


/** PaperSize handles paper sizes. It uses the standard called "PWG 5101.1-2002 PWG: Standard for Media Standardized Names" 
 * to name the paper sizes (and to get the data for the page sizes). In addition to standard paper sizes, PaperSize allows 
 * to construct custom paper sizes with arbitrary dimensions.
 *
 * The PaperSize object stores not only the dimensions (width and height) of a paper size and its name, it also provides 
 * default print margins. 
 *
 * @newin2p10
 *
 * @ingroup Printing
 */
class PaperSize
{
  // Cannot pass the _new function here, it must accept the 'name' argument.
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef PaperSize CppObjectType;
  typedef GtkPaperSize BaseObjectType;

  static GType get_type() G_GNUC_CONST;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  PaperSize();

  explicit PaperSize(GtkPaperSize* gobject, bool make_a_copy = true);

  PaperSize(const PaperSize& other);
  PaperSize& operator=(const PaperSize& other);

  ~PaperSize();

  void swap(PaperSize& other);

  ///Provides access to the underlying C instance.
  GtkPaperSize*       gobj()       { return gobject_; }

  ///Provides access to the underlying C instance.
  const GtkPaperSize* gobj() const { return gobject_; }

  ///Provides access to the underlying C instance. The caller is responsible for freeing it. Use when directly setting fields in structs.
  GtkPaperSize* gobj_copy() const;

protected:
  GtkPaperSize* gobject_;

private:

public:
  // Allowing no argument works because gtk_paper_size_new() will call
  // gtk_paper_size_get_default() in case of NULL.
  /// If a name is not specified, a default value will be used.
  explicit PaperSize(const Glib::ustring& name = "");
  explicit PaperSize(const Glib::ustring& ppd_name, const Glib::ustring& ppd_display_name, double width, double height);
  explicit PaperSize(const Glib::ustring& name, const Glib::ustring& display_name, double width, double height, Unit unit);

  
  bool equal(const PaperSize& other) const;

  
  Glib::ustring get_name() const;
  
  Glib::ustring get_display_name() const;
  
  Glib::ustring get_ppd_name() const;

  
  double get_width(Unit unit) const;
  
  double get_height(Unit unit) const;
  
  bool is_custom() const;

  
  void set_size(double width, double height, Unit unit);
  
  double get_default_top_margin(Unit unit) const;
  
  double get_default_bottom_margin(Unit unit) const;
  
  double get_default_left_margin(Unit unit) const;
  
  double get_default_right_margin(Unit unit) const;

  
  static Glib::ustring get_default();


};

/** @relates Gtk::PaperSize */
inline bool operator==(const PaperSize& lhs, const PaperSize& rhs)
  { return lhs.equal(rhs); }

/** @relates Gtk::PaperSize */
inline bool operator!=(const PaperSize& lhs, const PaperSize& rhs)
  { return !lhs.equal(rhs); }

} // namespace Gtk


namespace Gtk
{

/** @relates Gtk::PaperSize
 * @param lhs The left-hand side
 * @param rhs The right-hand side
 */
inline void swap(PaperSize& lhs, PaperSize& rhs)
  { lhs.swap(rhs); }

} // namespace Gtk

namespace Glib
{

/** @relates Gtk::PaperSize
 * @param object The C instance
 * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
 * @result A C++ instance that wraps this C instance.
 */
Gtk::PaperSize wrap(GtkPaperSize* object, bool take_copy = false);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <>
class Value<Gtk::PaperSize> : public Glib::Value_Boxed<Gtk::PaperSize>
{};
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

} // namespace Glib


#endif /* _GTKMM_PAPERSIZE_H */

