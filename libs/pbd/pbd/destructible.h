/*
    Copyright (C) 2000-2007 Paul Davis 

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

#ifndef __pbd_destructible_h__
#define __pbd_destructible_h__

#include <sigc++/signal.h>

namespace PBD {

/* be very very careful using this class. it does not inherit from sigc::trackable and thus
   should only be used in multiple-inheritance situations involving another type
   that does inherit from sigc::trackable (or sigc::trackable itself)
*/

class ThingWithGoingAway {
  public:
	virtual ~ThingWithGoingAway () {}
	sigc::signal<void> GoingAway;
};

class Destructible : public sigc::trackable, public ThingWithGoingAway {
  public:
	virtual ~Destructible () {}
	void drop_references () const { GoingAway(); }

};

}

#endif /* __pbd_destructible_h__ */
