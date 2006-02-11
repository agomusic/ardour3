/*
    Copyright (C) 2002 Paul Davis 

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

    $Id$
*/

#include "ardour/port.h"

using namespace ARDOUR;
using namespace std;

Port::Port (jack_port_t *p) 
	: port (p)
{
	if (port == 0) {
		throw failed_constructor();
	}
	
	_flags = JackPortFlags (jack_port_flags (port));
	_type  = jack_port_type (port); 
	_name = jack_port_name (port);

	reset ();
}

void
Port::reset ()
{
	reset_buffer ();
	
	last_monitor = false;
	silent = false;
	metering = 0;
	
	reset_meters ();
}

int 
Port::set_name (string str)
{
	int ret;

	if ((ret = jack_port_set_name (port, str.c_str())) == 0) {
		_name = str;
	}
	
	return ret;
}

	
