/*
	Copyright (C) 2006,2007 John Anderson

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
#include "surface_port.h"

#include "mackie_control_exception.h"
#include "controls.h"

#include <midi++/types.h>
#include <midi++/port.h>
#include <sigc++/sigc++.h>
#include <boost/shared_array.hpp>

#include "i18n.h"

#include <sstream>

#include <cstring>
#include <cerrno>

using namespace std;
using namespace Mackie;

SurfacePort::SurfacePort()
: _port( 0 ), _number( 0 ), _active( false )
{
}

SurfacePort::SurfacePort( MIDI::Port & port, int number )
: _port( &port ), _number( number ), _active( false )
{
}

SurfacePort::~SurfacePort()
{
#ifdef PORT_DEBUG
	cout << "~SurfacePort::SurfacePort()" << endl;
#endif
	// make sure another thread isn't reading or writing as we close the port
	Glib::RecMutex::Lock lock( _rwlock );
	_active = false;
#ifdef PORT_DEBUG
	cout << "~SurfacePort::SurfacePort() finished" << endl;
#endif
}

// wrapper for one day when strerror_r is working properly
string fetch_errmsg( int error_number )
{
	char * msg = strerror( error_number );
	return msg;
}
	
MidiByteArray SurfacePort::read()
{
	const int max_buf_size = 512;
	MIDI::byte buf[max_buf_size];
	MidiByteArray retval;

	// check active. Mainly so that the destructor
	// doesn't destroy the mutex while it's still locked
	if ( !active() ) return retval;
	
	// return nothing read if the lock isn't acquired
#if 0
	Glib::RecMutex::Lock lock( _rwlock, Glib::TRY_LOCK );
		
	if ( !lock.locked() )
	{
		cout << "SurfacePort::read not locked" << endl;
		return retval;
	}
	
	// check active again - destructor sequence
	if ( !active() ) return retval;
#endif
	
	// read port and copy to return value
	int nread = port().read( buf, sizeof (buf) );

	if (nread >= 0) {
		retval.copy( nread, buf );
		if ((size_t) nread == sizeof (buf))
		{
#ifdef PORT_DEBUG
			cout << "SurfacePort::read recursive" << endl;
#endif
			retval << read();
		}
	}
	else
	{
		if ( errno != EAGAIN )
		{
			ostringstream os;
			os << "Surface: error reading from port: " << port().name();
			os << ": " << errno << fetch_errmsg( errno );

			cout << os.str() << endl;
			inactive_event();
			throw MackieControlException( os.str() );
		}
	}
#ifdef PORT_DEBUG
	cout << "SurfacePort::read: " << retval << endl;
#endif
	return retval;
}

void SurfacePort::write( const MidiByteArray & mba )
{
#ifdef PORT_DEBUG
	cout << "SurfacePort::write: " << mba << endl;
#endif
	
	// check active before and after lock - to make sure
	// that the destructor doesn't destroy the mutex while
	// it's still in use
	if ( !active() ) return;
	Glib::RecMutex::Lock lock( _rwlock );
	if ( !active() ) return;

	int count = port().write( mba.bytes().get(), mba.size(), 0);
	if ( count != (int)mba.size() )
	{
		if ( errno == 0 )
		{
			cout << "port overflow on " << port().name() << ". Did not write all of " << mba << endl;
		}
		else if ( errno != EAGAIN )
		{
			ostringstream os;
			os << "Surface: couldn't write to port " << port().name();
			os << ", error: " << fetch_errmsg( errno ) << "(" << errno << ")";
			
			cout << os.str() << endl;
			inactive_event();
		}
	}
#ifdef PORT_DEBUG
	cout << "SurfacePort::wrote " << count << endl;
#endif
}

void SurfacePort::write_sysex( const MidiByteArray & mba )
{
	MidiByteArray buf;
	buf << sysex_hdr() << mba << MIDI::eox;
	write( buf );
}

void SurfacePort::write_sysex( MIDI::byte msg )
{
	MidiByteArray buf;
	buf << sysex_hdr() << msg << MIDI::eox;
	write( buf );
}

ostream & Mackie::operator << ( ostream & os, const SurfacePort & port )
{
	os << "{ ";
	os << "device: " << port.port().device();
	os << "; ";
	os << "name: " << port.port().name();
	os << "; ";
	os << " }";
	return os;
}
