/*
    Copyright (C) 2009 Paul Davis 

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

#include "global_port_matrix.h"
#include "i18n.h"
#include "ardour/bundle.h"
#include "ardour/session.h"
#include "ardour/audioengine.h"
#include "ardour/port.h"

GlobalPortMatrix::GlobalPortMatrix (ARDOUR::Session& s, ARDOUR::DataType t)
	: PortMatrix (s, t, true, PortGroupList::Mask (PortGroupList::BUSS |
						       PortGroupList::TRACK |
						       PortGroupList::SYSTEM | 
						       PortGroupList::OTHER)),
	  _session (s),
	  _our_port_group_list (s, t, false, PortGroupList::Mask (PortGroupList::BUSS |
								  PortGroupList::TRACK |
								  PortGroupList::SYSTEM | 
								  PortGroupList::OTHER))
{
	setup ();

	_port_group_list.VisibilityChanged.connect (sigc::mem_fun (*this, &GlobalPortMatrix::group_visibility_changed));
}

void
GlobalPortMatrix::group_visibility_changed ()
{
	_our_port_group_list.take_visibility_from (_port_group_list);
	setup ();
}


void
GlobalPortMatrix::setup ()
{
	_our_port_group_list.refresh ();
	_our_bundles = _our_port_group_list.bundles ();
	
	PortMatrix::setup ();
	
}

void
GlobalPortMatrix::set_state (
	boost::shared_ptr<ARDOUR::Bundle> ab,
	uint32_t ac,
	boost::shared_ptr<ARDOUR::Bundle> bb,
	uint32_t bc,
	bool s,
	uint32_t k
	)
{
	ARDOUR::Bundle::PortList const& our_ports = ab->channel_ports (ac);
	ARDOUR::Bundle::PortList const& other_ports = bb->channel_ports (bc);

	for (ARDOUR::Bundle::PortList::const_iterator i = our_ports.begin(); i != our_ports.end(); ++i) {
		for (ARDOUR::Bundle::PortList::const_iterator j = other_ports.begin(); j != other_ports.end(); ++j) {

			ARDOUR::Port* p = _session.engine().get_port_by_name (*i);
			ARDOUR::Port* q = _session.engine().get_port_by_name (*j);

			if (p) {
				if (s) {
					p->connect (*j);
				} else {
					p->disconnect (*j);
				}
			} else if (q) {
				if (s) {
					q->connect (*i);
				} else {
					q->disconnect (*j);
				}
			}

			/* we don't handle connections between two non-Ardour ports */
		}
	}
}


PortMatrix::State
GlobalPortMatrix::get_state (
	boost::shared_ptr<ARDOUR::Bundle> ab,
	uint32_t ac,
	boost::shared_ptr<ARDOUR::Bundle> bb,
	uint32_t bc
	) const
{
	ARDOUR::Bundle::PortList const& our_ports = ab->channel_ports (ac);
	ARDOUR::Bundle::PortList const& other_ports = bb->channel_ports (bc);

	for (ARDOUR::Bundle::PortList::const_iterator i = our_ports.begin(); i != our_ports.end(); ++i) {
		for (ARDOUR::Bundle::PortList::const_iterator j = other_ports.begin(); j != other_ports.end(); ++j) {

			ARDOUR::Port* p = _session.engine().get_port_by_name (*i);
			ARDOUR::Port* q = _session.engine().get_port_by_name (*j);

			/* we don't know the state of connections between two non-Ardour ports */
			if (!p && !q) {
				return UNKNOWN;
			}

			if (p && p->connected_to (*j) == false) {
				return NOT_ASSOCIATED;
			} else if (q && q->connected_to (*i) == false) {
				return NOT_ASSOCIATED;
			}

		}
	}

	return ASSOCIATED;
}


GlobalPortMatrixWindow::GlobalPortMatrixWindow (ARDOUR::Session& s, ARDOUR::DataType t)
	: ArdourDialog (
		t == ARDOUR::DataType::AUDIO ?
		_("Audio Connections Manager") :
		_("MIDI Connections Manager")),
	  _port_matrix (s, t)
{
	get_vbox()->pack_start (_port_matrix);
	show_all ();
}
