/*
    Copyright (C) 2002-2009 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream>
#include "port_matrix_component.h"

/** Constructor.
 *  @param p Port matrix that we're in.
 */
PortMatrixComponent::PortMatrixComponent (PortMatrixBody* b)
	: _body (b),
	  _pixmap (0),
	  _render_required (true),
	  _dimension_computation_required (true)
{
	
}

/** Destructor */
PortMatrixComponent::~PortMatrixComponent ()
{
	if (_pixmap) {
		gdk_pixmap_unref (_pixmap);
	}
}

void
PortMatrixComponent::setup ()
{
	_dimension_computation_required = true;
	_render_required = true;
}

GdkPixmap *
PortMatrixComponent::get_pixmap (GdkDrawable *drawable)
{
	if (_render_required) {

		if (_dimension_computation_required) {
			compute_dimensions ();
			_dimension_computation_required = false;
		}

		/* we may be zero width or height; if so, just
		   use the smallest allowable pixmap */
		if (_width == 0) {
			_width = 1;
		}
		if (_height == 0) {
			_height = 1;
		}

		/* make a pixmap of the right size */
		if (_pixmap) {
			gdk_pixmap_unref (_pixmap);
		}
		_pixmap = gdk_pixmap_new (drawable, _width, _height, -1);

		/* render */
		cairo_t* cr = gdk_cairo_create (_pixmap);
		render (cr);
		cairo_destroy (cr);

		_render_required = false;
	}

	return _pixmap;
}

void
PortMatrixComponent::set_source_rgb (cairo_t *cr, Gdk::Color const & c)
{
	cairo_set_source_rgb (cr, c.get_red_p(), c.get_green_p(), c.get_blue_p());
}

void
PortMatrixComponent::set_source_rgba (cairo_t *cr, Gdk::Color const & c, double a)
{
	cairo_set_source_rgba (cr, c.get_red_p(), c.get_green_p(), c.get_blue_p(), a);
}

std::pair<uint32_t, uint32_t>
PortMatrixComponent::dimensions ()
{
	if (_dimension_computation_required) {
		compute_dimensions ();
		_dimension_computation_required = false;
	}

	return std::make_pair (_width, _height);
}
