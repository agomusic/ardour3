/*
    Copyright (C) 2006 Paul Davis 

    This program is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser
    General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id$
*/

#ifndef ardour_control_protocols_h
#define ardour_control_protocols_h

#include <string>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>
#include <pbd/stateful.h>
#include <control_protocol/basic_ui.h>

namespace ARDOUR {

class Route;
class Session;

class ControlProtocol : public sigc::trackable, public Stateful, public BasicUI {
  public:
	ControlProtocol (Session&, std::string name);
	virtual ~ControlProtocol();

	std::string name() const { return _name; }

	virtual int set_active (bool yn) = 0;
	bool get_active() const { return _active; }

	sigc::signal<void> ActiveChanged;

	/* signals that a control protocol can emit and other (presumably graphical)
	   user interfaces can respond to
	*/

	static sigc::signal<void> ZoomToSession;
	static sigc::signal<void> ZoomIn;
	static sigc::signal<void> ZoomOut;
	static sigc::signal<void> Enter;
	static sigc::signal<void,float> ScrollTimeline;

	/* the model here is as follows:

	   we imagine most control surfaces being able to control
	   from 1 to N tracks at a time, with a session that may
	   contain 1 to M tracks, where M may be smaller, larger or
	   equal to N. 

	   the control surface has a fixed set of physical controllers
	   which can potentially be mapped onto different tracks/busses
	   via some mechanism.

	   therefore, the control protocol object maintains
	   a table that reflects the current mapping between
	   the controls and route object.
	*/

	void set_route_table_size (uint32_t size);
	void set_route_table (uint32_t table_index, boost::shared_ptr<ARDOUR::Route>);
	bool set_route_table (uint32_t table_index, uint32_t remote_control_id);

	void route_set_rec_enable (uint32_t table_index, bool yn);
	bool route_get_rec_enable (uint32_t table_index);

	float route_get_gain (uint32_t table_index);
	void route_set_gain (uint32_t table_index, float);
	float route_get_effective_gain (uint32_t table_index);

	float route_get_peak_input_power (uint32_t table_index, uint32_t which_input);

	bool route_get_muted (uint32_t table_index);
	void route_set_muted (uint32_t table_index, bool);

	bool route_get_soloed (uint32_t table_index);
	void route_set_soloed (uint32_t table_index, bool);

	std::string route_get_name (uint32_t table_index);

  protected:
	std::vector<boost::shared_ptr<ARDOUR::Route> > route_table;
	std::string _name;
	bool _active;

	void next_track (uint32_t initial_id);
	void prev_track (uint32_t initial_id);
};

extern "C" {
	struct ControlProtocolDescriptor {
	    const char* name;      /* descriptive */
	    const char* id;        /* unique and version-specific */
	    void*       ptr;       /* protocol can store a value here */
	    void*       module;    /* not for public access */
	    int         mandatory; /* if non-zero, always load and do not make optional */
	    bool             (*probe)(ControlProtocolDescriptor*);
	    ControlProtocol* (*initialize)(ControlProtocolDescriptor*,Session*);
	    void             (*destroy)(ControlProtocolDescriptor*,ControlProtocol*);
	    
	};
}

}

#endif // ardour_control_protocols_h
