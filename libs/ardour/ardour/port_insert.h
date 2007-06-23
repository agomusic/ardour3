/*
    Copyright (C) 2000,2007 Paul Davis 

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

#ifndef __ardour_port_insert_h__
#define __ardour_port_insert_h__

#include <vector>
#include <string>
#include <exception>

#include <sigc++/signal.h>
#include <ardour/ardour.h>
#include <ardour/redirect.h>
#include <ardour/plugin_state.h>
#include <ardour/types.h>

class XMLNode;

namespace ARDOUR {

class Session;

/** Port inserts: send output to a Jack port, pick up input at a Jack port
 *
 * PortInsert IS-A Redirect IS-A Insert, IO
 */
class PortInsert : public Redirect
{
  public:
	PortInsert (Session&, Placement);
	PortInsert (Session&, const XMLNode&);
	PortInsert (const PortInsert&);
	~PortInsert ();

	XMLNode& state(bool full);
	XMLNode& get_state(void);
	int set_state(const XMLNode&);

	void init ();
	
	void run (BufferSet& bufs, nframes_t start_frame, nframes_t end_frame, nframes_t nframes, nframes_t offset);

	nframes_t latency();
	
	ChanCount output_streams() const;
	ChanCount input_streams() const;

	virtual bool      can_support_input_configuration (ChanCount in) const;
	virtual ChanCount output_for_input_configuration (ChanCount in) const;
	virtual bool      configure_io (ChanCount in, ChanCount out);

	uint32_t bit_slot() const { return bitslot; }

  private:
	uint32_t bitslot;
};

} // namespace ARDOUR

#endif /* __ardour_port_insert_h__ */
