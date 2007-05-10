/*
    Copyright (C) 2000 Paul Davis 

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

#include <algorithm>

#include <pbd/xml++.h>

#include <ardour/send.h>
#include <ardour/session.h>
#include <ardour/port.h>
#include <ardour/audio_port.h>
#include <ardour/buffer_set.h>
#include <ardour/meter.h>
#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;

Send::Send (Session& s, Placement p)
	: Redirect (s, string_compose (_("send %1"), (bitslot = s.next_send_id()) + 1), p)
{
	_metering = false;
	RedirectCreated (this); /* EMIT SIGNAL */
}

Send::Send (Session& s, const XMLNode& node)
	: Redirect (s,  "send", PreFader)
{
	_metering = false;

	if (set_state (node)) {
		throw failed_constructor();
	}

	RedirectCreated (this); /* EMIT SIGNAL */
}

Send::Send (const Send& other)
	: Redirect (other._session, string_compose (_("send %1"), (bitslot = other._session.next_send_id()) + 1), other.placement())
{
	_metering = false;
	RedirectCreated (this); /* EMIT SIGNAL */
}

Send::~Send ()
{
	GoingAway ();
}

XMLNode&
Send::get_state(void)
{
	return state (true);
}

XMLNode&
Send::state(bool full)
{
	XMLNode *node = new XMLNode("Send");
	char buf[32];
	node->add_child_nocopy (Redirect::state (full));
	snprintf (buf, sizeof (buf), "%" PRIu32, bitslot);
	node->add_property ("bitslot", buf);
	return *node;
}

int
Send::set_state(const XMLNode& node)
{
	XMLNodeList nlist = node.children();
	XMLNodeIterator niter;
	const XMLProperty* prop;

	if ((prop = node.property ("bitslot")) == 0) {
		bitslot = _session.next_send_id();
	} else {
		sscanf (prop->value().c_str(), "%" PRIu32, &bitslot);
		_session.mark_send_id (bitslot);
	}

	/* Send has regular IO automation (gain, pan) */

	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		if ((*niter)->name() == Redirect::state_node_name) {
			Redirect::set_state (**niter);
			break;
		} else if ((*niter)->name() == X_("Automation")) {
			IO::set_automation_state (*(*niter));
		}
	}

	if (niter == nlist.end()) {
		error << _("XML node describing a send is missing a Redirect node") << endmsg;
		return -1;
	}

	return 0;
}

void
Send::run (BufferSet& bufs, nframes_t start_frame, nframes_t end_frame, nframes_t nframes, nframes_t offset)
{
	if (active()) {

		// we have to copy the input, because IO::deliver_output may alter the buffers
		// in-place, which a send must never do.

		BufferSet& sendbufs = _session.get_send_buffers(bufs.count());

		sendbufs.read_from(bufs, nframes);
		assert(sendbufs.count() == bufs.count());

		IO::deliver_output (sendbufs, start_frame, end_frame, nframes, offset);

		if (_metering) {
			if (_gain == 0) {
				_meter->reset();
			} else {
				_meter->run(output_buffers(), nframes, offset);
			}
		}

	} else {
		silence (nframes, offset);
		
		if (_metering) {
			_meter->reset();
		}
	}
}

void
Send::set_metering (bool yn)
{
	_metering = yn;

	if (!_metering) {
		/* XXX possible thread hazard here */
		peak_meter().reset();
	}
}

void
Send::expect_inputs (const ChanCount& expected)
{
	if (expected != _expected_inputs) {
		_expected_inputs = expected;
		reset_panner ();
	}
}
