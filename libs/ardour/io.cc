/*
    Copyright (C) 2000-2006 Paul Davis 

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

#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <locale.h>

#include <sigc++/bind.h>

#include <glibmm/thread.h>

#include <pbd/xml++.h>

#include <ardour/audioengine.h>
#include <ardour/io.h>
#include <ardour/port.h>
#include <ardour/audio_port.h>
#include <ardour/midi_port.h>
#include <ardour/connection.h>
#include <ardour/session.h>
#include <ardour/cycle_timer.h>
#include <ardour/panner.h>
#include <ardour/buffer_set.h>
#include <ardour/meter.h>
#include <ardour/amp.h>

#include "i18n.h"

#include <cmath>

/*
  A bug in OS X's cmath that causes isnan() and isinf() to be 
  "undeclared". the following works around that
*/

#if defined(__APPLE__) && defined(__MACH__)
extern "C" int isnan (double);
extern "C" int isinf (double);
#endif


using namespace std;
using namespace ARDOUR;
using namespace PBD;


static float current_automation_version_number = 1.0;

jack_nframes_t               IO::_automation_interval = 0;
const string                 IO::state_node_name = "IO";
bool                         IO::connecting_legal = false;
bool                         IO::ports_legal = false;
bool                         IO::panners_legal = false;
sigc::signal<void>           IO::Meter;
sigc::signal<int>            IO::ConnectingLegal;
sigc::signal<int>            IO::PortsLegal;
sigc::signal<int>            IO::PannersLegal;
sigc::signal<void,ChanCount> IO::MoreChannels;
sigc::signal<int>            IO::PortsCreated;

Glib::StaticMutex IO::m_meter_signal_lock = GLIBMM_STATIC_MUTEX_INIT;

/* this is a default mapper of [0 .. 1.0] control values to a gain coefficient.
   others can be imagined. 
*/

static gain_t direct_control_to_gain (double fract) { 
	/* XXX Marcus writes: this doesn't seem right to me. but i don't have a better answer ... */
	/* this maxes at +6dB */
	return pow (2.0,(sqrt(sqrt(sqrt(fract)))*198.0-192.0)/6.0);
}

static double direct_gain_to_control (gain_t gain) { 
	/* XXX Marcus writes: this doesn't seem right to me. but i don't have a better answer ... */
	if (gain == 0) return 0.0;
	
	return pow((6.0*log(gain)/log(2.0)+192.0)/198.0, 8.0);
}


/** @param default_type The type of port that will be created by ensure_io
 * and friends if no type is explicitly requested (to avoid breakage).
 */
IO::IO (Session& s, string name,
	int input_min, int input_max, int output_min, int output_max,
	DataType default_type)
	: _session (s),
      _output_buffers(new BufferSet()),
	  _name (name),
	  _default_type(default_type),
	  _gain_control (*this),
	  _gain_automation_curve (0.0, 2.0, 1.0),
	  _input_minimum (_default_type, input_min),
	  _input_maximum (_default_type, input_max),
	  _output_minimum (_default_type, output_min),
	  _output_maximum (_default_type, output_max)
{
	_panner = new Panner (name, _session);
	_meter = new PeakMeter (_session);

	_gain = 1.0;
	_desired_gain = 1.0;
	_input_connection = 0;
	_output_connection = 0;
	pending_state_node = 0;
	no_panner_reset = false;
	_phase_invert = false;
	deferred_state = 0;

	apply_gain_automation = false;

	last_automation_snapshot = 0;

	_gain_automation_state = Off;
	_gain_automation_style = Absolute;

	{
		// IO::Meter is emitted from another thread so the
		// Meter signal must be protected.
		Glib::Mutex::Lock guard (m_meter_signal_lock);
		m_meter_connection = Meter.connect (mem_fun (*this, &IO::meter));
	}
	
	// Connect to our own MoreChannels signal to connect output buffers
	IO::MoreChannels.connect (mem_fun (*this, &IO::attach_buffers));
}

IO::~IO ()
{
	Glib::Mutex::Lock guard (m_meter_signal_lock);
	
	Glib::Mutex::Lock lm (io_lock);

	for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
		_session.engine().unregister_port (*i);
	}

	for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		_session.engine().unregister_port (*i);
	}

	m_meter_connection.disconnect();

	delete _meter;
	delete _panner;
	delete _output_buffers;
}

void
IO::silence (jack_nframes_t nframes, jack_nframes_t offset)
{
	/* io_lock, not taken: function must be called from Session::process() calltree */

	for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		i->silence (nframes, offset);
	}
}

/** Deliver bufs to the IO's Jack outputs.
 *
 * This function should automatically do whatever it necessary to correctly deliver bufs
 * to the outputs, eg applying gain or pan or whatever else needs to be done.
 */
void
IO::deliver_output (BufferSet& bufs, jack_nframes_t start_frame, jack_nframes_t end_frame, jack_nframes_t nframes, jack_nframes_t offset)
{
	// FIXME: type specific code doesn't actually need to be here, it will go away in time
	

	/* ********** AUDIO ********** */

	// Apply gain if gain automation isn't playing
	if ( ! apply_gain_automation) {
		
		gain_t dg = _gain; // desired gain

		{
			Glib::Mutex::Lock dm (declick_lock, Glib::TRY_LOCK);

			if (dm.locked()) {
				dg = _desired_gain;
			}
		}

		Amp::run(bufs, nframes, _gain, dg, _phase_invert);
	}
	
	// Use the panner to distribute audio to output port buffers
	if (_panner && !_panner->empty() && !_panner->bypassed()) {
		_panner->distribute(bufs, output_buffers(), start_frame, end_frame, nframes, offset);
	}


	/* ********** MIDI ********** */

	// No MIDI, we're done here
	if (bufs.count().get(DataType::MIDI) == 0) {
		return;
	}

	const DataType type = DataType::MIDI;
	
	// Just dump any MIDI 1-to-1, we're not at all clever with MIDI routing yet
	BufferSet::iterator o = output_buffers().begin(type);
	for (BufferSet::iterator i = bufs.begin(type); i != bufs.end(type); ++i, ++o) {
		o->read_from(*i, nframes, offset);
	}
}

void
IO::collect_input (BufferSet& outs, jack_nframes_t nframes, jack_nframes_t offset)
{
	outs.set_count(n_inputs());
	
	if (outs.count() == ChanCount::ZERO)
		return;

	for (DataType::iterator t = DataType::begin(); t != DataType::end(); ++t) {
		
		BufferSet::iterator o = outs.begin(*t);
		for (PortSet::iterator i = _inputs.begin(*t); i != _inputs.end(*t); ++i, ++o) {
			o->read_from(i->get_buffer(), nframes, offset);
		}

	}
}

void
IO::just_meter_input (jack_nframes_t start_frame, jack_nframes_t end_frame, 
		      jack_nframes_t nframes, jack_nframes_t offset)
{
	BufferSet& bufs = _session.get_scratch_buffers ();
	ChanCount nbufs = n_process_buffers ();

	collect_input (bufs, nframes, offset);

	_meter->run(bufs, nframes);
}

void
IO::drop_input_connection ()
{
	_input_connection = 0;
	input_connection_configuration_connection.disconnect();
	input_connection_connection_connection.disconnect();
	_session.set_dirty ();
}

void
IO::drop_output_connection ()
{
	_output_connection = 0;
	output_connection_configuration_connection.disconnect();
	output_connection_connection_connection.disconnect();
	_session.set_dirty ();
}

int
IO::disconnect_input (Port* our_port, string other_port, void* src)
{
	if (other_port.length() == 0 || our_port == 0) {
		return 0;
	}

	{ 
		Glib::Mutex::Lock em (_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			/* check that our_port is really one of ours */
			
			if ( ! _inputs.contains(our_port)) {
				return -1;
			}
			
			/* disconnect it from the source */
			
			if (_session.engine().disconnect (other_port, our_port->name())) {
				error << string_compose(_("IO: cannot disconnect input port %1 from %2"), our_port->name(), other_port) << endmsg;
				return -1;
			}

			drop_input_connection();
		}
	}

	input_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();

	return 0;
}

int
IO::connect_input (Port* our_port, string other_port, void* src)
{
	if (other_port.length() == 0 || our_port == 0) {
		return 0;
	}

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			/* check that our_port is really one of ours */
			
			if ( ! _inputs.contains(our_port) ) {
				return -1;
			}
			
			/* connect it to the source */

			if (_session.engine().connect (other_port, our_port->name())) {
				return -1;
			}
			
			drop_input_connection ();
		}
	}

	input_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	return 0;
}

int
IO::disconnect_output (Port* our_port, string other_port, void* src)
{
	if (other_port.length() == 0 || our_port == 0) {
		return 0;
	}

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			/* check that our_port is really one of ours */
			
			if ( ! _outputs.contains(our_port) ) {
				return -1;
			}
			
			/* disconnect it from the destination */
			
			if (_session.engine().disconnect (our_port->name(), other_port)) {
				error << string_compose(_("IO: cannot disconnect output port %1 from %2"), our_port->name(), other_port) << endmsg;
				return -1;
			}

			drop_output_connection ();
		}
	}

	output_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	return 0;
}

int
IO::connect_output (Port* our_port, string other_port, void* src)
{
	if (other_port.length() == 0 || our_port == 0) {
		return 0;
	}

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			/* check that our_port is really one of ours */
			
			if ( ! _outputs.contains(our_port) ) {
				return -1;
			}
			
			/* connect it to the destination */
			
			if (_session.engine().connect (our_port->name(), other_port)) {
				return -1;
			}

			drop_output_connection ();
		}
	}

	output_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	return 0;
}

int
IO::set_input (Port* other_port, void* src)
{
	/* this removes all but one ports, and connects that one port
	   to the specified source.
	*/

	if (_input_minimum.get_total() > 1) {
		/* sorry, you can't do this */
		return -1;
	}

	if (other_port == 0) {
		if (_input_minimum == ChanCount::ZERO) {
			return ensure_inputs (0, false, true, src);
		} else {
			return -1;
		}
	}

	if (ensure_inputs (1, true, true, src)) {
		return -1;
	}

	return connect_input (_inputs.port(0), other_port->name(), src);
}

int
IO::remove_output_port (Port* port, void* src)
{
	throw; // FIXME
#if 0
	IOChange change (NoChange);

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			if (_noutputs - 1 == (uint32_t) _output_minimum) {
				/* sorry, you can't do this */
				return -1;
			}
			
			for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
				if (*i == port) {
					change = IOChange (change|ConfigurationChanged);
					if (port->connected()) {
						change = IOChange (change|ConnectionsChanged);
					} 

					_session.engine().unregister_port (*i);
					_outputs.erase (i);
					_noutputs--;
					drop_output_connection ();

					break;
				}
			}

			if (change != NoChange) {
				setup_peak_meters ();
				reset_panner ();
			}
		}
	}
	
	if (change != NoChange) {
		output_changed (change, src); /* EMIT SIGNAL */
		_session.set_dirty ();
		return 0;
	}
#endif
	return -1;
}

/** Add an output port.
 *
 * @param destination Name of input port to connect new port to.
 * @param src Source for emitted ConfigurationChanged signal.
 * @param type Data type of port.  Default value (NIL) will use this IO's default type.
 */
int
IO::add_output_port (string destination, void* src, DataType type)
{
	Port* our_port;
	char name[64];

	if (type == DataType::NIL)
		type = _default_type;

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{ 
			Glib::Mutex::Lock lm (io_lock);
			
			if (n_outputs() >= _output_maximum) {
				return -1;
			}
		
			/* Create a new output port */
			
			// FIXME: naming scheme for differently typed ports?
			if (_output_maximum.get_total() == 1) {
				snprintf (name, sizeof (name), _("%s/out"), _name.c_str());
			} else {
				snprintf (name, sizeof (name), _("%s/out %u"), _name.c_str(), find_output_port_hole());
			}
			
			if ((our_port = _session.engine().register_output_port (type, name)) == 0) {
				error << string_compose(_("IO: cannot register output port %1"), name) << endmsg;
				return -1;
			}
			
			_outputs.add_port (our_port);
			drop_output_connection ();
			setup_peak_meters ();
			reset_panner ();
		}

		MoreChannels (n_outputs()); /* EMIT SIGNAL */
	}

	if (destination.length()) {
		if (_session.engine().connect (our_port->name(), destination)) {
			return -1;
		}
	}
	
	// pan_changed (src); /* EMIT SIGNAL */
	output_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	
	return 0;
}

int
IO::remove_input_port (Port* port, void* src)
{
	throw; // FIXME
#if 0
	IOChange change (NoChange);

	{
		Glib::Mutex::Lock em(_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);

			if (((int)_ninputs - 1) < _input_minimum) {
				/* sorry, you can't do this */
				return -1;
			}
			for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {

				if (*i == port) {
					change = IOChange (change|ConfigurationChanged);

					if (port->connected()) {
						change = IOChange (change|ConnectionsChanged);
					} 

					_session.engine().unregister_port (*i);
					_inputs.erase (i);
					_ninputs--;
					drop_input_connection ();

					break;
				}
			}
			
			if (change != NoChange) {
				setup_peak_meters ();
				reset_panner ();
			}
		}
	}

	if (change != NoChange) {
		input_changed (change, src);
		_session.set_dirty ();
		return 0;
	} 
#endif
	return -1;
}


/** Add an input port.
 *
 * @param type Data type of port.  The appropriate Jack port type, and @ref Port will be created.
 * @param destination Name of input port to connect new port to.
 * @param src Source for emitted ConfigurationChanged signal.
 */
int
IO::add_input_port (string source, void* src, DataType type)
{
	Port* our_port;
	char name[64];
	
	if (type == DataType::NIL)
		type = _default_type;

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		
		{ 
			Glib::Mutex::Lock lm (io_lock);
			
			if (n_inputs() >= _input_maximum) {
				return -1;
			}

			/* Create a new input port */
			
			// FIXME: naming scheme for differently typed ports?
			if (_input_maximum.get_total() == 1) {
				snprintf (name, sizeof (name), _("%s/in"), _name.c_str());
			} else {
				snprintf (name, sizeof (name), _("%s/in %u"), _name.c_str(), find_input_port_hole());
			}
			
			if ((our_port = _session.engine().register_input_port (type, name)) == 0) {
				error << string_compose(_("IO: cannot register input port %1"), name) << endmsg;
				return -1;
			}
			
			_inputs.add_port(our_port);
			drop_input_connection ();
			setup_peak_meters ();
			reset_panner ();
		}

		MoreChannels (n_inputs()); /* EMIT SIGNAL */
	}

	if (source.length()) {

		if (_session.engine().connect (source, our_port->name())) {
			return -1;
		}
	} 

	// pan_changed (src); /* EMIT SIGNAL */
	input_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	
	return 0;
}

int
IO::disconnect_inputs (void* src)
{
	{ 
		Glib::Mutex::Lock em (_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
				_session.engine().disconnect (*i);
			}

			drop_input_connection ();
		}
	}
	
	input_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	
	return 0;
}

int
IO::disconnect_outputs (void* src)
{
	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		
		{
			Glib::Mutex::Lock lm (io_lock);
			
			for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
				_session.engine().disconnect (*i);
			}

			drop_output_connection ();
		}
	}

	output_changed (ConnectionsChanged, src); /* EMIT SIGNAL */
	_session.set_dirty ();
	
	return 0;
}

bool
IO::ensure_inputs_locked (uint32_t n, bool clear, void* src)
{
	Port* input_port;
	bool changed = false;
	
	/* remove unused ports */

	while (n_inputs().get(_default_type) > n) {
		throw; // FIXME
		/*
		_session.engine().unregister_port (_inputs.back());
		_inputs.pop_back();
		_ninputs--;
		changed = true;
		*/
	}
		
	/* create any necessary new ports */
		
	while (n_inputs().get(_default_type) < n) {
		
		char buf[64];
		
		/* Create a new input port (of the default type) */
		
		if (_input_maximum.get_total() == 1) {
			snprintf (buf, sizeof (buf), _("%s/in"), _name.c_str());
		}
		else {
			snprintf (buf, sizeof (buf), _("%s/in %u"), _name.c_str(), find_input_port_hole());
		}
		
		try {
			
			if ((input_port = _session.engine().register_input_port (_default_type, buf)) == 0) {
				error << string_compose(_("IO: cannot register input port %1"), buf) << endmsg;
				return -1;
			}
		}

		catch (AudioEngine::PortRegistrationFailure& err) {
			setup_peak_meters ();
			reset_panner ();
			/* pass it on */
			throw err;
		}
		
		_inputs.add_port (input_port);
		changed = true;
	}
	
	if (changed) {
		drop_input_connection ();
		setup_peak_meters ();
		reset_panner ();
		MoreChannels (n_inputs()); /* EMIT SIGNAL */
		_session.set_dirty ();
	}
	
	if (clear) {
		/* disconnect all existing ports so that we get a fresh start */
			
		for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
			_session.engine().disconnect (*i);
		}
	}

	return changed;
}

/** Attach output_buffers to port buffers.
 * 
 * Connected to IOs own MoreChannels signal.
 */
void
IO::attach_buffers(ChanCount ignored)
{
	_output_buffers->attach_buffers(_outputs);
}

int
IO::ensure_io (const ChanCount& in, const ChanCount& out, bool clear, void* src)
{
	// FIXME: TYPE
	uint32_t nin = in.get(_default_type);
	uint32_t nout = out.get(_default_type);

	// We only deal with one type still.  Sorry about your luck.
	assert(nin == in.get_total());
	assert(nout == out.get_total());

	return ensure_io(nin, nout, clear, src);
}

int
IO::ensure_io (uint32_t nin, uint32_t nout, bool clear, void* src)
{
	bool in_changed = false;
	bool out_changed = false;
	bool need_pan_reset;

	nin = min (_input_maximum.get(_default_type), static_cast<size_t>(nin));

	nout = min (_output_maximum.get(_default_type), static_cast<size_t>(nout));

	if (nin == n_inputs().get(_default_type) && nout == n_outputs().get(_default_type) && !clear) {
		return 0;
	}

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		Glib::Mutex::Lock lm (io_lock);

		Port* port;
		
		if (n_outputs().get(_default_type) == nout) {
			need_pan_reset = false;
		} else {
			need_pan_reset = true;
		}
		
		/* remove unused ports */
		
		while (n_inputs().get(_default_type) > nin) {
			throw; // FIXME
			/*
			_session.engine().unregister_port (_inputs.back());
			_inputs.pop_back();
			_ninputs--;
			in_changed = true;*/
		}
		
		while (n_outputs().get(_default_type) > nout) {
			throw; // FIXME
			/*
			_session.engine().unregister_port (_outputs.back());
			_outputs.pop_back();
			_noutputs--;
			out_changed = true;*/
		}
		
		/* create any necessary new ports (of the default type) */
		
		while (n_inputs().get(_default_type) < nin) {
			
			char buf[64];

			/* Create a new input port */
			
			if (_input_maximum.get_total() == 1) {
				snprintf (buf, sizeof (buf), _("%s/in"), _name.c_str());
			}
			else {
				snprintf (buf, sizeof (buf), _("%s/in %u"), _name.c_str(), find_input_port_hole());
			}
			
			try {
				if ((port = _session.engine().register_input_port (_default_type, buf)) == 0) {
					error << string_compose(_("IO: cannot register input port %1"), buf) << endmsg;
					return -1;
				}
			}

			catch (AudioEngine::PortRegistrationFailure& err) {
				setup_peak_meters ();
				reset_panner ();
				/* pass it on */
				throw err;
			}
		
			_inputs.add_port (port);
			in_changed = true;
		}

		/* create any necessary new ports */
		
		while (n_outputs().get(_default_type) < nout) {
			
			char buf[64];
			
			/* Create a new output port */
			
			if (_output_maximum.get_total() == 1) {
				snprintf (buf, sizeof (buf), _("%s/out"), _name.c_str());
			} else {
				snprintf (buf, sizeof (buf), _("%s/out %u"), _name.c_str(), find_output_port_hole());
			}
			
			try { 
				if ((port = _session.engine().register_output_port (_default_type, buf)) == 0) {
					error << string_compose(_("IO: cannot register output port %1"), buf) << endmsg;
					return -1;
				}
			}
			
			catch (AudioEngine::PortRegistrationFailure& err) {
				setup_peak_meters ();
				reset_panner ();
				/* pass it on */
				throw err;
			}
		
			_outputs.add_port (port);
			out_changed = true;
		}
		
		if (clear) {
			
			/* disconnect all existing ports so that we get a fresh start */
			
			for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
				_session.engine().disconnect (*i);
			}
			
			for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
				_session.engine().disconnect (*i);
			}
		}
		
		if (in_changed || out_changed) {
			setup_peak_meters ();
			reset_panner ();
		}
	}

	if (out_changed) {
		drop_output_connection ();
		output_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
	}
	
	if (in_changed) {
		drop_input_connection ();
		input_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
	}

	if (in_changed || out_changed) {
		MoreChannels (max (n_outputs(), n_inputs())); /* EMIT SIGNAL */
		_session.set_dirty ();
	}

	return 0;
}

int
IO::ensure_inputs (uint32_t n, bool clear, bool lockit, void* src)
{
	bool changed = false;

	n = min (_input_maximum.get(_default_type), static_cast<size_t>(n));

	if (n == n_inputs().get(_default_type) && !clear) {
		return 0;
	}

	if (lockit) {
		Glib::Mutex::Lock em (_session.engine().process_lock());
		Glib::Mutex::Lock im (io_lock);
		changed = ensure_inputs_locked (n, clear, src);
	} else {
		changed = ensure_inputs_locked (n, clear, src);
	}

	if (changed) {
		input_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
		_session.set_dirty ();
	}
	return 0;
}

bool
IO::ensure_outputs_locked (uint32_t n, bool clear, void* src)
{
	Port* output_port;
	bool changed = false;
	bool need_pan_reset;

	if (n_outputs().get(_default_type) == n) {
		need_pan_reset = false;
	} else {
		need_pan_reset = true;
	}
	
	/* remove unused ports */
	
	while (n_outputs().get(_default_type) > n) {
		throw; // FIXME
		/*
		_session.engine().unregister_port (_outputs.back());
		_outputs.pop_back();
		_noutputs--;
		changed = true;
		*/
	}
	
	/* create any necessary new ports */
	
	while (n_outputs().get(_default_type) < n) {
		
		char buf[64];
		
		/* Create a new output port */
		
		if (_output_maximum.get(_default_type) == 1) {
			snprintf (buf, sizeof (buf), _("%s/out"), _name.c_str());
		} else {
			snprintf (buf, sizeof (buf), _("%s/out %u"), _name.c_str(), find_output_port_hole());
		}
		
		if ((output_port = _session.engine().register_output_port (_default_type, buf)) == 0) {
			error << string_compose(_("IO: cannot register output port %1"), buf) << endmsg;
			return -1;
		}
		
		_outputs.add_port (output_port);
		changed = true;
		setup_peak_meters ();

		if (need_pan_reset) {
			reset_panner ();
		}
	}
	
	if (changed) {
		drop_output_connection ();
		MoreChannels (n_outputs()); /* EMIT SIGNAL */
		_session.set_dirty ();
	}
	
	if (clear) {
		/* disconnect all existing ports so that we get a fresh start */
		
		for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
			_session.engine().disconnect (*i);
		}
	}

	return changed;
}

int
IO::ensure_outputs (uint32_t n, bool clear, bool lockit, void* src)
{
	bool changed = false;

	if (_output_maximum < ChanCount::INFINITE) {
		n = min (_output_maximum.get(_default_type), static_cast<size_t>(n));
		if (n == n_outputs().get(_default_type) && !clear) {
			return 0;
		}
	}

	/* XXX caller should hold io_lock, but generally doesn't */

	if (lockit) {
		Glib::Mutex::Lock em (_session.engine().process_lock());
		Glib::Mutex::Lock im (io_lock);
		changed = ensure_outputs_locked (n, clear, src);
	} else {
		changed = ensure_outputs_locked (n, clear, src);
	}

	if (changed) {
		 output_changed (ConfigurationChanged, src); /* EMIT SIGNAL */
	}
	return 0;
}

gain_t
IO::effective_gain () const
{
	if (gain_automation_playback()) {
		return _effective_gain;
	} else {
		return _desired_gain;
	}
}

void
IO::reset_panner ()
{
	if (panners_legal) {
		if (!no_panner_reset) {
			_panner->reset (n_outputs().get(_default_type), pans_required());
		}
	} else {
		panner_legal_c.disconnect ();
		panner_legal_c = PannersLegal.connect (mem_fun (*this, &IO::panners_became_legal));
	}
}

int
IO::panners_became_legal ()
{
	_panner->reset (n_outputs().get(_default_type), pans_required());
	_panner->load (); // automation
	panner_legal_c.disconnect ();
	return 0;
}

void
IO::defer_pan_reset ()
{
	no_panner_reset = true;
}

void
IO::allow_pan_reset ()
{
	no_panner_reset = false;
	reset_panner ();
}


XMLNode&
IO::get_state (void)
{
	return state (true);
}

XMLNode&
IO::state (bool full_state)
{
	XMLNode* node = new XMLNode (state_node_name);
	char buf[64];
	string str;
	bool need_ins = true;
	bool need_outs = true;
	LocaleGuard lg (X_("POSIX"));
	Glib::Mutex::Lock lm (io_lock);

	node->add_property("name", _name);
	id().print (buf);
	node->add_property("id", buf);

	str = "";

	if (_input_connection) {
		node->add_property ("input-connection", _input_connection->name());
		need_ins = false;
	}

	if (_output_connection) {
		node->add_property ("output-connection", _output_connection->name());
		need_outs = false;
	}

	if (need_ins) {
		for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
			
			const char **connections = i->get_connections();
			
			if (connections && connections[0]) {
				str += '{';
				
				for (int n = 0; connections && connections[n]; ++n) {
					if (n) {
						str += ',';
					}
					
					/* if its a connection to our own port,
					   return only the port name, not the
					   whole thing. this allows connections
					   to be re-established even when our
					   client name is different.
					*/
					
					str += _session.engine().make_port_name_relative (connections[n]);
				}	

				str += '}';
				
				free (connections);
			}
			else {
				str += "{}";
			}
		}
		
		node->add_property ("inputs", str);
	}

	if (need_outs) {
		str = "";
		
		for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
			
			const char **connections = i->get_connections();
			
			if (connections && connections[0]) {
				
				str += '{';
				
				for (int n = 0; connections[n]; ++n) {
					if (n) {
						str += ',';
					}

					str += _session.engine().make_port_name_relative (connections[n]);
				}

				str += '}';
				
				free (connections);
			}
			else {
				str += "{}";
			}
		}
		
		node->add_property ("outputs", str);
	}

	node->add_child_nocopy (_panner->state (full_state));

	snprintf (buf, sizeof(buf), "%2.12f", gain());
	node->add_property ("gain", buf);

	const int in_min  = (_input_minimum == ChanCount::ZERO) ? -1 : _input_minimum.get(_default_type);
	const int in_max  = (_input_maximum == ChanCount::INFINITE) ? -1 : _input_maximum.get(_default_type);
	const int out_min = (_output_minimum == ChanCount::ZERO) ? -1 : _output_minimum.get(_default_type);
	const int out_max = (_output_maximum == ChanCount::INFINITE) ? -1 : _output_maximum.get(_default_type);

	snprintf (buf, sizeof(buf)-1, "%d,%d,%d,%d", in_min, in_max, out_min, out_max);

	node->add_property ("iolimits", buf);

	/* automation */

	if (full_state) {
		snprintf (buf, sizeof (buf), "0x%x", (int) _gain_automation_curve.automation_state());
	} else {
		/* never store anything except Off for automation state in a template */
		snprintf (buf, sizeof (buf), "0x%x", ARDOUR::Off); 
	}
	node->add_property ("automation-state", buf);
	snprintf (buf, sizeof (buf), "0x%x", (int) _gain_automation_curve.automation_style());
	node->add_property ("automation-style", buf);

	/* XXX same for pan etc. */

	return *node;
}

int
IO::connecting_became_legal ()
{
	int ret;

	if (pending_state_node == 0) {
		fatal << _("IO::connecting_became_legal() called without a pending state node") << endmsg;
		/*NOTREACHED*/
		return -1;
	}

	connection_legal_c.disconnect ();

	ret = make_connections (*pending_state_node);

	if (ports_legal) {
		delete pending_state_node;
		pending_state_node = 0;
	}

	return ret;
}

int
IO::ports_became_legal ()
{
	int ret;

	if (pending_state_node == 0) {
		fatal << _("IO::ports_became_legal() called without a pending state node") << endmsg;
		/*NOTREACHED*/
		return -1;
	}

	port_legal_c.disconnect ();

	ret = create_ports (*pending_state_node);

	if (connecting_legal) {
		delete pending_state_node;
		pending_state_node = 0;
	}

	return ret;
}

int
IO::set_state (const XMLNode& node)
{
	const XMLProperty* prop;
	XMLNodeConstIterator iter;
	LocaleGuard lg (X_("POSIX"));

	/* force use of non-localized representation of decimal point,
	   since we use it a lot in XML files and so forth.
	*/

	if (node.name() != state_node_name) {
		error << string_compose(_("incorrect XML node \"%1\" passed to IO object"), node.name()) << endmsg;
		return -1;
	}

	if ((prop = node.property ("name")) != 0) {
		_name = prop->value();
		_panner->set_name (_name);
	} 

	if ((prop = node.property ("id")) != 0) {
		_id = prop->value ();
	}

	size_t in_min =  -1;
	size_t in_max  = -1;
	size_t out_min = -1;
	size_t out_max = -1;

	if ((prop = node.property ("iolimits")) != 0) {
		sscanf (prop->value().c_str(), "%zd,%zd,%zd,%zd",
			&in_min, &in_max, &out_min, &out_max);
		_input_minimum = ChanCount(_default_type, in_min);
		_input_maximum = ChanCount(_default_type, in_max);
		_output_minimum = ChanCount(_default_type, out_min);
		_output_maximum = ChanCount(_default_type, out_max);
	}
	
	if ((prop = node.property ("gain")) != 0) {
		set_gain (atof (prop->value().c_str()), this);
		_gain = _desired_gain;
	}

	for (iter = node.children().begin(); iter != node.children().end(); ++iter) {
		if ((*iter)->name() == "Panner") {
			_panner->set_state (**iter);
		}
	}

	if ((prop = node.property ("automation-state")) != 0) {

		long int x;
		x = strtol (prop->value().c_str(), 0, 16);
		set_gain_automation_state (AutoState (x));
	}

	if ((prop = node.property ("automation-style")) != 0) {

	       long int x;
		x = strtol (prop->value().c_str(), 0, 16);
		set_gain_automation_style (AutoStyle (x));
	}
	
	if (ports_legal) {

		if (create_ports (node)) {
			return -1;
		}

	} else {

		port_legal_c = PortsLegal.connect (mem_fun (*this, &IO::ports_became_legal));
	}

	if (panners_legal) {
		reset_panner ();
	} else {
		panner_legal_c = PannersLegal.connect (mem_fun (*this, &IO::panners_became_legal));
	}

	if (connecting_legal) {

		if (make_connections (node)) {
			return -1;
		}

	} else {
		
		connection_legal_c = ConnectingLegal.connect (mem_fun (*this, &IO::connecting_became_legal));
	}

	if (!ports_legal || !connecting_legal) {
		pending_state_node = new XMLNode (node);
	}

	return 0;
}

int
IO::create_ports (const XMLNode& node)
{
	const XMLProperty* prop;
	int num_inputs = 0;
	int num_outputs = 0;

	if ((prop = node.property ("input-connection")) != 0) {

		Connection* c = _session.connection_by_name (prop->value());
		
		if (c == 0) {
			error << string_compose(_("Unknown connection \"%1\" listed for input of %2"), prop->value(), _name) << endmsg;

			if ((c = _session.connection_by_name (_("in 1"))) == 0) {
				error << _("No input connections available as a replacement")
				      << endmsg;
				return -1;
			}  else {
				info << string_compose (_("Connection %1 was not available - \"in 1\" used instead"), prop->value())
				     << endmsg;
			}
		} 

		num_inputs = c->nports();

	} else if ((prop = node.property ("inputs")) != 0) {

		num_inputs = count (prop->value().begin(), prop->value().end(), '{');
	}
	
	if ((prop = node.property ("output-connection")) != 0) {
		Connection* c = _session.connection_by_name (prop->value());

		if (c == 0) {
			error << string_compose(_("Unknown connection \"%1\" listed for output of %2"), prop->value(), _name) << endmsg;

			if ((c = _session.connection_by_name (_("out 1"))) == 0) {
				error << _("No output connections available as a replacement")
				      << endmsg;
				return -1;
			}  else {
				info << string_compose (_("Connection %1 was not available - \"out 1\" used instead"), prop->value())
				     << endmsg;
			}
		} 

		num_outputs = c->nports ();
		
	} else if ((prop = node.property ("outputs")) != 0) {
		num_outputs = count (prop->value().begin(), prop->value().end(), '{');
	}

	no_panner_reset = true;

	if (ensure_io (num_inputs, num_outputs, true, this)) {
		error << string_compose(_("%1: cannot create I/O ports"), _name) << endmsg;
		return -1;
	}

	no_panner_reset = false;

	set_deferred_state ();

	PortsCreated();
	return 0;
}


int
IO::make_connections (const XMLNode& node)
{
	const XMLProperty* prop;

	if ((prop = node.property ("input-connection")) != 0) {
		Connection* c = _session.connection_by_name (prop->value());
		
		if (c == 0) {
			error << string_compose(_("Unknown connection \"%1\" listed for input of %2"), prop->value(), _name) << endmsg;

			if ((c = _session.connection_by_name (_("in 1"))) == 0) {
				error << _("No input connections available as a replacement")
				      << endmsg;
				return -1;
			} else {
				info << string_compose (_("Connection %1 was not available - \"in 1\" used instead"), prop->value())
				     << endmsg;
			}
		} 

		use_input_connection (*c, this);

	} else if ((prop = node.property ("inputs")) != 0) {
		if (set_inputs (prop->value())) {
			error << string_compose(_("improper input channel list in XML node (%1)"), prop->value()) << endmsg;
			return -1;
		}
	}
	
	if ((prop = node.property ("output-connection")) != 0) {
		Connection* c = _session.connection_by_name (prop->value());
		
		if (c == 0) {
			error << string_compose(_("Unknown connection \"%1\" listed for output of %2"), prop->value(), _name) << endmsg;

			if ((c = _session.connection_by_name (_("out 1"))) == 0) {
				error << _("No output connections available as a replacement")
				      << endmsg;
				return -1;
			}  else {
				info << string_compose (_("Connection %1 was not available - \"out 1\" used instead"), prop->value())
				     << endmsg;
			}
		} 

		use_output_connection (*c, this);
		
	} else if ((prop = node.property ("outputs")) != 0) {
		if (set_outputs (prop->value())) {
			error << string_compose(_("improper output channel list in XML node (%1)"), prop->value()) << endmsg;
			return -1;
		}
	}
	
	return 0;
}

int
IO::set_inputs (const string& str)
{
	vector<string> ports;
	int i;
	int n;
	uint32_t nports;
	
	if ((nports = count (str.begin(), str.end(), '{')) == 0) {
		return 0;
	}

	if (ensure_inputs (nports, true, true, this)) {
		return -1;
	}

	string::size_type start, end, ostart;

	ostart = 0;
	start = 0;
	end = 0;
	i = 0;

	while ((start = str.find_first_of ('{', ostart)) != string::npos) {
		start += 1;

		if ((end = str.find_first_of ('}', start)) == string::npos) {
			error << string_compose(_("IO: badly formed string in XML node for inputs \"%1\""), str) << endmsg;
			return -1;
		}

		if ((n = parse_io_string (str.substr (start, end - start), ports)) < 0) {
			error << string_compose(_("bad input string in XML node \"%1\""), str) << endmsg;

			return -1;
			
		} else if (n > 0) {

			for (int x = 0; x < n; ++x) {
				connect_input (input (i), ports[x], this);
			}
		}

		ostart = end+1;
		i++;
	}

	return 0;
}

int
IO::set_outputs (const string& str)
{
	vector<string> ports;
	int i;
	int n;
	uint32_t nports;
	
	if ((nports = count (str.begin(), str.end(), '{')) == 0) {
		return 0;
	}

	if (ensure_outputs (nports, true, true, this)) {
		return -1;
	}

	string::size_type start, end, ostart;

	ostart = 0;
	start = 0;
	end = 0;
	i = 0;

	while ((start = str.find_first_of ('{', ostart)) != string::npos) {
		start += 1;

		if ((end = str.find_first_of ('}', start)) == string::npos) {
			error << string_compose(_("IO: badly formed string in XML node for outputs \"%1\""), str) << endmsg;
			return -1;
		}

		if ((n = parse_io_string (str.substr (start, end - start), ports)) < 0) {
			error << string_compose(_("IO: bad output string in XML node \"%1\""), str) << endmsg;

			return -1;
			
		} else if (n > 0) {

			for (int x = 0; x < n; ++x) {
				connect_output (output (i), ports[x], this);
			}
		}

		ostart = end+1;
		i++;
	}

	return 0;
}

int
IO::parse_io_string (const string& str, vector<string>& ports)
{
	string::size_type pos, opos;

	if (str.length() == 0) {
		return 0;
	}

	pos = 0;
	opos = 0;

	ports.clear ();

	while ((pos = str.find_first_of (',', opos)) != string::npos) {
		ports.push_back (str.substr (opos, pos - opos));
		opos = pos + 1;
	}
	
	if (opos < str.length()) {
		ports.push_back (str.substr(opos));
	}

	return ports.size();
}

int
IO::parse_gain_string (const string& str, vector<string>& ports)
{
	string::size_type pos, opos;

	pos = 0;
	opos = 0;
	ports.clear ();

	while ((pos = str.find_first_of (',', opos)) != string::npos) {
		ports.push_back (str.substr (opos, pos - opos));
		opos = pos + 1;
	}
	
	if (opos < str.length()) {
		ports.push_back (str.substr(opos));
	}

	return ports.size();
}

int
IO::set_name (string name, void* src)
{
	if (name == _name) {
		return 0;
	}

	for (PortSet::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
		string current_name = i->short_name();
		current_name.replace (current_name.find (_name), _name.length(), name);
		i->set_name (current_name);
	}

	for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		string current_name = i->short_name();
		current_name.replace (current_name.find (_name), _name.length(), name);
		i->set_name (current_name);
	}

	_name = name;
	 name_changed (src); /* EMIT SIGNAL */

	 return 0;
}

void
IO::set_input_minimum (int n)
{
	if (n < 0)
		_input_minimum = ChanCount::ZERO;
	else
		_input_minimum = ChanCount(_default_type, n);
}

void
IO::set_input_maximum (int n)
{
	if (n < 0)
		_input_maximum = ChanCount::INFINITE;
	else
		_input_maximum = ChanCount(_default_type, n);
}

void
IO::set_output_minimum (int n)
{
	if (n < 0)
		_output_minimum = ChanCount::ZERO;
	else
		_output_minimum = ChanCount(_default_type, n);
}

void
IO::set_output_maximum (int n)
{
	if (n < 0)
		_output_maximum = ChanCount::INFINITE;
	else
		_output_maximum = ChanCount(_default_type, n);
}

void
IO::set_input_minimum (ChanCount n)
{
	_input_minimum = n;
}

void
IO::set_input_maximum (ChanCount n)
{
	_input_maximum = n;
}

void
IO::set_output_minimum (ChanCount n)
{
	_output_minimum = n;
}

void
IO::set_output_maximum (ChanCount n)
{
	_output_maximum = n;
}

void
IO::set_port_latency (jack_nframes_t nframes)
{
	Glib::Mutex::Lock lm (io_lock);

	for (PortSet::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		i->set_latency (nframes);
	}
}

jack_nframes_t
IO::output_latency () const
{
	jack_nframes_t max_latency;
	jack_nframes_t latency;

	max_latency = 0;

	/* io lock not taken - must be protected by other means */

	for (PortSet::const_iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		if ((latency = _session.engine().get_port_total_latency (*i)) > max_latency) {
			max_latency = latency;
		}
	}

	return max_latency;
}

jack_nframes_t
IO::input_latency () const
{
	jack_nframes_t max_latency;
	jack_nframes_t latency;

	max_latency = 0;

	/* io lock not taken - must be protected by other means */

	for (PortSet::const_iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
		if ((latency = _session.engine().get_port_total_latency (*i)) > max_latency) {
			max_latency = latency;
		}
	}

	return max_latency;
}

int
IO::use_input_connection (Connection& c, void* src)
{
	uint32_t limit;

	{
		Glib::Mutex::Lock lm (_session.engine().process_lock());
		Glib::Mutex::Lock lm2 (io_lock);
		
		limit = c.nports();
		
		drop_input_connection ();
		
		if (ensure_inputs (limit, false, false, src)) {
			return -1;
		}

		/* first pass: check the current state to see what's correctly
		   connected, and drop anything that we don't want.
		*/
		
		for (uint32_t n = 0; n < limit; ++n) {
			const Connection::PortList& pl = c.port_connections (n);
			
			for (Connection::PortList::const_iterator i = pl.begin(); i != pl.end(); ++i) {
				
				if (!_inputs.port(n)->connected_to ((*i))) {
					
					/* clear any existing connections */
					
					_session.engine().disconnect (*_inputs.port(n));
					
				} else if (_inputs.port(n)->connected() > 1) {
					
					/* OK, it is connected to the port we want,
					   but its also connected to other ports.
					   Change that situation.
					*/
					
					/* XXX could be optimized to not drop
					   the one we want.
					*/
					
					_session.engine().disconnect (*_inputs.port(n));
					
				}
			}
		}
		
		/* second pass: connect all requested ports where necessary */
		
		for (uint32_t n = 0; n < limit; ++n) {
			const Connection::PortList& pl = c.port_connections (n);
			
			for (Connection::PortList::const_iterator i = pl.begin(); i != pl.end(); ++i) {
				
				if (!_inputs.port(n)->connected_to ((*i))) {
					
					if (_session.engine().connect (*i, _inputs.port(n)->name())) {
						return -1;
					}
				}
				
			}
		}
		
		_input_connection = &c;
		
		input_connection_configuration_connection = c.ConfigurationChanged.connect
			(mem_fun (*this, &IO::input_connection_configuration_changed));
		input_connection_connection_connection = c.ConnectionsChanged.connect
			(mem_fun (*this, &IO::input_connection_connection_changed));
	}

	input_changed (IOChange (ConfigurationChanged|ConnectionsChanged), src); /* EMIT SIGNAL */
	return 0;
}

int
IO::use_output_connection (Connection& c, void* src)
{
	uint32_t limit;	

	{
		Glib::Mutex::Lock lm (_session.engine().process_lock());
		Glib::Mutex::Lock lm2 (io_lock);

		limit = c.nports();
			
		drop_output_connection ();

		if (ensure_outputs (limit, false, false, src)) {
			return -1;
		}

		/* first pass: check the current state to see what's correctly
		   connected, and drop anything that we don't want.
		*/
			
		for (uint32_t n = 0; n < limit; ++n) {

			const Connection::PortList& pl = c.port_connections (n);
				
			for (Connection::PortList::const_iterator i = pl.begin(); i != pl.end(); ++i) {
					
				if (!_outputs.port(n)->connected_to ((*i))) {

					/* clear any existing connections */

					_session.engine().disconnect (*_outputs.port(n));

				} else if (_outputs.port(n)->connected() > 1) {

					/* OK, it is connected to the port we want,
					   but its also connected to other ports.
					   Change that situation.
					*/

					/* XXX could be optimized to not drop
					   the one we want.
					*/
						
					_session.engine().disconnect (*_outputs.port(n));
				}
			}
		}

		/* second pass: connect all requested ports where necessary */

		for (uint32_t n = 0; n < limit; ++n) {

			const Connection::PortList& pl = c.port_connections (n);
				
			for (Connection::PortList::const_iterator i = pl.begin(); i != pl.end(); ++i) {
					
				if (!_outputs.port(n)->connected_to ((*i))) {
						
					if (_session.engine().connect (_outputs.port(n)->name(), *i)) {
						return -1;
					}
				}
			}
		}

		_output_connection = &c;

		output_connection_configuration_connection = c.ConfigurationChanged.connect
			(mem_fun (*this, &IO::output_connection_configuration_changed));
		output_connection_connection_connection = c.ConnectionsChanged.connect
			(mem_fun (*this, &IO::output_connection_connection_changed));
	}

	output_changed (IOChange (ConnectionsChanged|ConfigurationChanged), src); /* EMIT SIGNAL */

	return 0;
}

int
IO::disable_connecting ()
{
	connecting_legal = false;
	return 0;
}

int
IO::enable_connecting ()
{
	connecting_legal = true;
	return ConnectingLegal ();
}

int
IO::disable_ports ()
{
	ports_legal = false;
	return 0;
}

int
IO::enable_ports ()
{
	ports_legal = true;
	return PortsLegal ();
}

int
IO::disable_panners (void)
{
	panners_legal = false;
	return 0;
}

int
IO::reset_panners ()
{
	panners_legal = true;
	return PannersLegal ();
}

void
IO::input_connection_connection_changed (int ignored)
{
	use_input_connection (*_input_connection, this);
}

void
IO::input_connection_configuration_changed ()
{
	use_input_connection (*_input_connection, this);
}

void
IO::output_connection_connection_changed (int ignored)
{
	use_output_connection (*_output_connection, this);
}

void
IO::output_connection_configuration_changed ()
{
	use_output_connection (*_output_connection, this);
}

void
IO::GainControllable::set_value (float val)
{
	io.set_gain (direct_control_to_gain (val), this);
}

float
IO::GainControllable::get_value (void) const
{
	return direct_gain_to_control (io.effective_gain());
}

UndoAction
IO::get_memento() const
{
  return sigc::bind (mem_fun (*(const_cast<IO *>(this)), &StateManager::use_state), _current_state_id);
}

Change
IO::restore_state (StateManager::State& state)
{
	return Change (0);
}

StateManager::State*
IO::state_factory (std::string why) const
{
	StateManager::State* state = new StateManager::State (why);
	return state;
}

void
IO::setup_peak_meters()
{
	_meter->setup(std::max(_inputs.count(), _outputs.count()));
}

/**
    Update the peak meters.

    The meter signal lock is taken to prevent modification of the 
    Meter signal while updating the meters, taking the meter signal
    lock prior to taking the io_lock ensures that all IO will remain 
    valid while metering.
*/   
void
IO::update_meters()
{
    Glib::Mutex::Lock guard (m_meter_signal_lock);
    
    Meter(); /* EMIT SIGNAL */
}

void
IO::meter ()
{
	// FIXME: Remove this function and just connect signal directly to PeakMeter::meter
	
	Glib::Mutex::Lock lm (io_lock); // READER: meter thread.
	_meter->meter();
}

int
IO::save_automation (const string& path)
{
	string fullpath;
	ofstream out;

	fullpath = _session.automation_dir();
	fullpath += path;

	out.open (fullpath.c_str());

	if (!out) {
		error << string_compose(_("%1: could not open automation event file \"%2\""), _name, fullpath) << endmsg;
		return -1;
	}

	out << X_("version ") << current_automation_version_number << endl;

	/* XXX use apply_to_points to get thread safety */
	
	for (AutomationList::iterator i = _gain_automation_curve.begin(); i != _gain_automation_curve.end(); ++i) {
		out << "g " << (jack_nframes_t) floor ((*i)->when) << ' ' << (*i)->value << endl;
	}

	_panner->save ();

	return 0;
}

int
IO::load_automation (const string& path)
{
	string fullpath;
	ifstream in;
	char line[128];
	uint32_t linecnt = 0;
	float version;
	LocaleGuard lg (X_("POSIX"));

	fullpath = _session.automation_dir();
	fullpath += path;

	in.open (fullpath.c_str());

	if (!in) {
		fullpath = _session.automation_dir();
		fullpath += _session.snap_name();
		fullpath += '-';
		fullpath += path;
		in.open (fullpath.c_str());
		if (!in) {
				error << string_compose(_("%1: cannot open automation event file \"%2\""), _name, fullpath) << endmsg;
				return -1;
		}
	}

	clear_automation ();

	while (in.getline (line, sizeof(line), '\n')) {
		char type;
		jack_nframes_t when;
		double value;

		if (++linecnt == 1) {
			if (memcmp (line, "version", 7) == 0) {
				if (sscanf (line, "version %f", &version) != 1) {
					error << string_compose(_("badly formed version number in automation event file \"%1\""), path) << endmsg;
					return -1;
				}
			} else {
				error << string_compose(_("no version information in automation event file \"%1\""), path) << endmsg;
				return -1;
			}

			if (version != current_automation_version_number) {
				error << string_compose(_("mismatched automation event file version (%1)"), version) << endmsg;
				return -1;
			}

			continue;
		}

		if (sscanf (line, "%c %" PRIu32 " %lf", &type, &when, &value) != 3) {
			warning << string_compose(_("badly formatted automation event record at line %1 of %2 (ignored)"), linecnt, path) << endmsg;
			continue;
		}

		switch (type) {
		case 'g':
			_gain_automation_curve.add (when, value, true);
			break;

		case 's':
			break;

		case 'm':
			break;

		case 'p':
			/* older (pre-1.0) versions of ardour used this */
			break;

		default:
			warning << _("dubious automation event found (and ignored)") << endmsg;
		}
	}

	_gain_automation_curve.save_state (_("loaded from disk"));

	return 0;
}
	
void
IO::clear_automation ()
{
	Glib::Mutex::Lock lm (automation_lock);
	_gain_automation_curve.clear ();
	_panner->clear_automation ();
}

void
IO::set_gain_automation_state (AutoState state)
{
	bool changed = false;

	{
		Glib::Mutex::Lock lm (automation_lock);

		if (state != _gain_automation_curve.automation_state()) {
			changed = true;
			last_automation_snapshot = 0;
			_gain_automation_curve.set_automation_state (state);
			
			if (state != Off) {
				set_gain (_gain_automation_curve.eval (_session.transport_frame()), this);
			}
		}
	}

	if (changed) {
		_session.set_dirty ();
		gain_automation_state_changed (); /* EMIT SIGNAL */
	}
}

void
IO::set_gain_automation_style (AutoStyle style)
{
	bool changed = false;

	{
		Glib::Mutex::Lock lm (automation_lock);

		if (style != _gain_automation_curve.automation_style()) {
			changed = true;
			_gain_automation_curve.set_automation_style (style);
		}
	}

	if (changed) {
		gain_automation_style_changed (); /* EMIT SIGNAL */
	}
}
void
IO::inc_gain (gain_t factor, void *src)
{
	if (_desired_gain == 0.0f)
		set_gain (0.000001f + (0.000001f * factor), src);
	else
		set_gain (_desired_gain + (_desired_gain * factor), src);
}

void
IO::set_gain (gain_t val, void *src)
{
	// max gain at about +6dB (10.0 ^ ( 6 dB * 0.05))
	if (val>1.99526231f) val=1.99526231f;

	{
		Glib::Mutex::Lock dm (declick_lock);
		_desired_gain = val;
	}

	if (_session.transport_stopped()) {
		_effective_gain = val;
		_gain = val;
	}

	gain_changed (src);
	_gain_control.Changed (); /* EMIT SIGNAL */
	
	if (_session.transport_stopped() && src != 0 && src != this && gain_automation_recording()) {
		_gain_automation_curve.add (_session.transport_frame(), val);
		
	}

	_session.set_dirty();
}

void
IO::start_gain_touch ()
{
	_gain_automation_curve.start_touch ();
}

void
IO::end_gain_touch ()
{
	_gain_automation_curve.stop_touch ();
}

void
IO::start_pan_touch (uint32_t which)
{
	if (which < _panner->size()) {
		(*_panner)[which]->automation().start_touch();
	}
}

void
IO::end_pan_touch (uint32_t which)
{
	if (which < _panner->size()) {
		(*_panner)[which]->automation().stop_touch();
	}

}

void
IO::automation_snapshot (jack_nframes_t now)
{
	if (last_automation_snapshot > now || (now - last_automation_snapshot) > _automation_interval) {

		if (gain_automation_recording()) {
			_gain_automation_curve.rt_add (now, gain());
		}
		
		_panner->snapshot (now);

		last_automation_snapshot = now;
	}
}

void
IO::transport_stopped (jack_nframes_t frame)
{
	_gain_automation_curve.reposition_for_rt_add (frame);

	if (_gain_automation_curve.automation_state() != Off) {
		
		if (gain_automation_recording()) {
			_gain_automation_curve.save_state (_("automation write/touch"));
		}

		/* the src=0 condition is a special signal to not propagate 
		   automation gain changes into the mix group when locating.
		*/

		set_gain (_gain_automation_curve.eval (frame), 0);
	}

	_panner->transport_stopped (frame);
}

int32_t
IO::find_input_port_hole ()
{
	/* CALLER MUST HOLD IO LOCK */

	uint32_t n;

	if (_inputs.empty()) {
		return 1;
	}

	for (n = 1; n < UINT_MAX; ++n) {
		char buf[jack_port_name_size()];
		PortSet::iterator i = _inputs.begin();

		snprintf (buf, jack_port_name_size(), _("%s/in %u"), _name.c_str(), n);

		for ( ; i != _inputs.end(); ++i) {
			if (i->short_name() == buf) {
				break;
			}
		}

		if (i == _inputs.end()) {
			break;
		}
	}
	return n;
}

int32_t
IO::find_output_port_hole ()
{
	/* CALLER MUST HOLD IO LOCK */

	uint32_t n;

	if (_outputs.empty()) {
		return 1;
	}

	for (n = 1; n < UINT_MAX; ++n) {
		char buf[jack_port_name_size()];
		PortSet::iterator i = _outputs.begin();

		snprintf (buf, jack_port_name_size(), _("%s/out %u"), _name.c_str(), n);

		for ( ; i != _outputs.end(); ++i) {
			if (i->short_name() == buf) {
				break;
			}
		}

		if (i == _outputs.end()) {
			break;
		}
	}
	
	return n;
}

AudioPort*
IO::audio_input(uint32_t n) const
{
	return dynamic_cast<AudioPort*>(input(n));
}

AudioPort*
IO::audio_output(uint32_t n) const
{
	return dynamic_cast<AudioPort*>(output(n));
}

MidiPort*
IO::midi_input(uint32_t n) const
{
	return dynamic_cast<MidiPort*>(input(n));
}

MidiPort*
IO::midi_output(uint32_t n) const
{
	return dynamic_cast<MidiPort*>(output(n));
}

void
IO::set_phase_invert (bool yn, void *src)
{
	if (_phase_invert != yn) {
		_phase_invert = yn;
	}
	//  phase_invert_changed (src); /* EMIT SIGNAL */
}

