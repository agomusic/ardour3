/*
    Copyright (C) 2006 Paul Davis 
    
    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.
    
    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <ardour/meter.h>
#include <algorithm>
#include <cmath>
#include <ardour/buffer_set.h>
#include <ardour/peak.h>
#include <ardour/dB.h>
#include <ardour/session.h>
#include <ardour/midi_buffer.h>
#include <ardour/audio_buffer.h>
#include <ardour/runtime_functions.h>

namespace ARDOUR {


/** Get peaks from @a bufs
 * Input acceptance is lenient - the first n buffers from @a bufs will
 * be metered, where n was set by the last call to setup(), excess meters will
 * be set to 0.
 */
void
PeakMeter::run_in_place (BufferSet& bufs, nframes_t start_frame, nframes_t end_frame, nframes_t nframes, nframes_t offset)
{
	uint32_t n = 0;
	uint32_t meterable = std::min(bufs.count().n_total(), (uint32_t)_peak_power.size());
	uint32_t limit = std::min (meterable, (uint32_t)bufs.count().n_midi());

	// Meter what we have (midi)
	for ( ; n < limit; ++n) {
	
		float val = 0;
		
		// GUI needs a better MIDI meter, not much information can be
		// expressed through peaks alone
		for (MidiBuffer::iterator i = bufs.get_midi(n).begin(); i != bufs.get_midi(n).end(); ++i) {
			const Evoral::MIDIEvent& ev = *i;
			if (ev.is_note_on()) {
				const float this_vel = log(ev.buffer()[2] / 127.0 * (M_E*M_E-M_E) + M_E) - 1.0;
				//printf("V %d -> %f\n", (int)((Byte)ev.buffer[2]), this_vel);
				if (this_vel > val)
					val = this_vel;
			} else {
				val += 1.0 / bufs.get_midi(n).capacity();
				if (val > 1.0)
					val = 1.0;
			}
		}
			
		_peak_power[n] = val;

	}
	
	limit = std::min (meterable, bufs.count().n_audio());

	// Meter what we have (audio)
	for ( ; n < limit; ++n) {
		_peak_power[n] = compute_peak (bufs.get_audio(n).data(nframes, offset), nframes, _peak_power[n]); 
	}

	// Zero any excess peaks
	for (size_t n = meterable; n < _peak_power.size(); ++n) {
		_peak_power[n] = 0;
	}
}

void
PeakMeter::reset ()
{
	for (size_t i = 0; i < _peak_power.size(); ++i) {
		_peak_power[i] = 0;
	}
}

void
PeakMeter::reset_max ()
{
	for (size_t i = 0; i < _max_peak_power.size(); ++i) {
		_max_peak_power[i] = -INFINITY;
	}
}

bool
PeakMeter::configure_io (ChanCount in, ChanCount out)
{
	/* we're transparent no matter what.  fight the power. */
	if (out != in) {
		return false;
	}

	uint32_t limit = in.n_total();

	while (_peak_power.size() > limit) {
		_peak_power.pop_back();
		_visible_peak_power.pop_back();
		_max_peak_power.pop_back();
	}

	while (_peak_power.size() < limit) {
		_peak_power.push_back(0);
		_visible_peak_power.push_back(minus_infinity());
		_max_peak_power.push_back(minus_infinity());
	}

	assert(_peak_power.size() == limit);
	assert(_visible_peak_power.size() == limit);
	assert(_max_peak_power.size() == limit);

	return Processor::configure_io (in, out);
}

/** To be driven by the Meter signal from IO.
 * Caller MUST hold io_lock!
 */
void
PeakMeter::meter ()
{
	assert(_visible_peak_power.size() == _peak_power.size());

	const size_t limit = _peak_power.size();

	for (size_t n = 0; n < limit; ++n) {

		/* XXX we should use atomic exchange here */

		/* grab peak since last read */

 		float new_peak = _peak_power[n];
		_peak_power[n] = 0;
		
		/* compute new visible value using falloff */

		if (new_peak > 0.0) {
			new_peak = coefficient_to_dB (new_peak);
		} else {
			new_peak = minus_infinity();
		}
		
		/* update max peak */
		
		_max_peak_power[n] = std::max (new_peak, _max_peak_power[n]);
		
		if (Config->get_meter_falloff() == 0.0f || new_peak > _visible_peak_power[n]) {
			_visible_peak_power[n] = new_peak;
		} else {
			// do falloff
			new_peak = _visible_peak_power[n] - (Config->get_meter_falloff() * 0.01f);
			_visible_peak_power[n] = std::max (new_peak, -INFINITY);
		}
	}
}

} // namespace ARDOUR
