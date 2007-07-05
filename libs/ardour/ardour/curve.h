/*
    Copyright (C) 2001-2007 Paul Davis 

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

#ifndef __ardour_curve_h__
#define __ardour_curve_h__

#include <sys/types.h>
#include <boost/utility.hpp>
#include <sigc++/signal.h>
#include <glibmm/thread.h>
#include <pbd/undo.h>
#include <list>
#include <algorithm>
#include <ardour/automation_event.h>

namespace ARDOUR {

class Curve : public boost::noncopyable
{
  public:
	Curve (const AutomationList& al);

	bool rt_safe_get_vector (double x0, double x1, float *arg, int32_t veclen);
	void get_vector (double x0, double x1, float *arg, int32_t veclen);

	void solve ();

  private:
	double unlocked_eval (double where);
	double multipoint_eval (double x);

	void _get_vector (double x0, double x1, float *arg, int32_t veclen);

	void on_list_dirty() { _dirty = true; }
	
	bool                  _dirty;
	const AutomationList& _list;
};

} // namespace ARDOUR

extern "C" {
	void curve_get_vector_from_c (void *arg, double, double, float*, int32_t);
}

#endif /* __ardour_curve_h__ */
