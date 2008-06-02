/*
    Copyright (C) 2007 Paul Davis 

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

#ifndef __ardour_auto_bundle_h__
#define __ardour_auto_bundle_h__

#include <vector>
#include <glibmm/thread.h>
#include "ardour/bundle.h"

namespace ARDOUR {

class AutoBundle : public Bundle {

  public:
	AutoBundle (bool i = true);
	AutoBundle (std::string const &, bool i = true);

	uint32_t nchannels () const;
	const PortList& channel_ports (uint32_t) const;

	void set_channels (uint32_t);
	void set_port (uint32_t, std::string const &);

  private:
	/// mutex for _ports;
	/// XXX: is this necessary?
	mutable Glib::Mutex _ports_mutex;
	std::vector<PortList> _ports;
};

}	
	
#endif /* __ardour_auto_bundle_h__ */
