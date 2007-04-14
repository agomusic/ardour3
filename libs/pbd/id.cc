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

#include <ostream>
#include <iostream>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <pbd/id.h>
#include <string>

using namespace std;
using namespace PBD;

Glib::Mutex* ID::counter_lock = 0;
uint64_t ID::_counter = 0;

void
ID::init ()
{
	counter_lock = new Glib::Mutex;
}

ID::ID ()
{
	Glib::Mutex::Lock lm (*counter_lock);
	_id = _counter++;
}

ID::ID (string str)
{
	string_assign (str);
}

int
ID::string_assign (string str)
{
	return sscanf (str.c_str(), "%" PRIu64, &_id) != 0;
}

void
ID::print (char* buf, uint32_t bufsize) const
{
	snprintf (buf, bufsize, "%" PRIu64, _id);
}

string ID::to_s() const
{
    char buf[32]; // see print()
    print(buf, sizeof (buf));
    return string(buf);
}

ID&
ID::operator= (string str)
{
	string_assign (str);
	return *this;
}

ostream&
operator<< (ostream& ostr, const ID& _id)
{
	char buf[32];
	_id.print (buf, sizeof (buf));
	ostr << buf;
	return ostr;
}

