/*
    Copyright (C) 2008 Hans Baier 

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

#include "ardour/ticker.h"
#include "ardour/session.h"
#include "ardour/tempo.h"

namespace ARDOUR
{


void Ticker::set_session(Session& s) 
{
	 _session = &s;
	 
	 if(_session) {
		 _session->tick.connect(mem_fun (*this, &Ticker::tick));
		 _session->GoingAway.connect(mem_fun (*this, &Ticker::going_away));
	 }
}

void MidiClockTicker::set_session(Session& s) 
{
	 Ticker::set_session(s);
	 
	 if(_session) {
		 _session->MIDIClock_PortChanged.connect(mem_fun (*this, &MidiClockTicker::update_midi_clock_port));
		 _session->TransportStateChange .connect(mem_fun (*this, &MidiClockTicker::transport_state_changed));
		 _session->PositionChanged      .connect(mem_fun (*this, &MidiClockTicker::position_changed));		 
		 _session->TransportLooped      .connect(mem_fun (*this, &MidiClockTicker::transport_looped));		 
		 update_midi_clock_port();
	 }
}

void MidiClockTicker::update_midi_clock_port()
{
	 _midi_port = _session->midi_clock_port();
}

void MidiClockTicker::transport_state_changed()
{
	float     speed     = _session->transport_speed();
	nframes_t position  = _session->transport_frame();
#ifdef DEBUG_MIDI_CLOCK	
	cerr << "Transport state change, speed:" << speed << "position:" << position<< " play loop " << _session->get_play_loop() << endl;
#endif	
	if (speed == 1.0f) {
		_last_tick = position;
		
		if (!Config->get_send_midi_clock()) 
			return;
		
		if (_session->get_play_loop()) {
			assert(_session->locations()->auto_loop_location());
			if (position == _session->locations()->auto_loop_location()->start()) {
				send_start_event(0);
			} else {
				send_continue_event(0);
			}
		} else if (position == 0) {
			send_start_event(0);
		} else {
			send_continue_event(0);
		}
		
		send_midi_clock_event(0);
		
	} else if (speed == 0.0f) {
		if (!Config->get_send_midi_clock()) 
			return;
		
		send_stop_event(0);
	}
	
	tick(position, *((ARDOUR::BBT_Time *) 0), *((SMPTE::Time *)0));
}

void MidiClockTicker::position_changed(nframes_t position)
{
#ifdef DEBUG_MIDI_CLOCK	
	cerr << "Position changed:" << position << endl;
#endif	
	_last_tick = position;
}

void MidiClockTicker::transport_looped()
{
	Location* loop_location = _session->locations()->auto_loop_location();
	assert(loop_location);

#ifdef DEBUG_MIDI_CLOCK	
	cerr << "Transport looped, position:" <<  _session->transport_frame()
	     << " loop start " << loop_location->start( )
	     << " loop end " << loop_location->end( )
	     << " play loop " << _session->get_play_loop()
	     <<  endl;
#endif
	
	// adjust _last_tick, so that the next MIDI clock message is sent 
	// in due time (and the tick interval is still constant)
	nframes_t elapsed_since_last_tick = loop_location->end() - _last_tick;
	_last_tick = loop_location->start() - elapsed_since_last_tick;
}

void MidiClockTicker::tick(const nframes_t& transport_frames, const BBT_Time& transport_bbt, const SMPTE::Time& transport_smpt)
{
#ifdef WITH_JACK_MIDI
	if (!Config->get_send_midi_clock() || _session == 0 || _session->transport_speed() != 1.0f)
		return;

	MIDI::JACK_MidiPort* jack_port = dynamic_cast<MIDI::JACK_MidiPort*>(_midi_port);
	assert(jack_port);
	
	while (true) {
		double next_tick = _last_tick + one_ppqn_in_frames(transport_frames);
		nframes_t next_tick_offset = nframes_t(next_tick) - transport_frames;
		
#ifdef DEBUG_MIDI_CLOCK	
		cerr << "Transport:" << transport_frames 
			 << ":Last tick time:" << _last_tick << ":" 
			 << ":Next tick time:" << next_tick << ":" 
			 << "Offset:" << next_tick_offset << ":"
			 << "cycle length:" << jack_port->nframes_this_cycle() 
			 << endl; 
#endif	
		
		if (next_tick_offset >= jack_port->nframes_this_cycle())
			return;
	
		send_midi_clock_event(next_tick_offset);
		
		_last_tick = next_tick;
	}
#endif // WITH_JACK_MIDI
}

double MidiClockTicker::one_ppqn_in_frames(nframes_t transport_position)
{
	const Tempo& current_tempo = _session->tempo_map().tempo_at(transport_position);
	const Meter& current_meter = _session->tempo_map().meter_at(transport_position);
	double frames_per_beat =
		current_tempo.frames_per_beat(_session->nominal_frame_rate(),
		                              current_meter);

	double quarter_notes_per_beat = 4.0 / current_tempo.note_type();
	double frames_per_quarter_note = frames_per_beat / quarter_notes_per_beat;

	return frames_per_quarter_note / double (_ppqn);
}

void MidiClockTicker::send_midi_clock_event(nframes_t offset)
{
#ifdef WITH_JACK_MIDI
	assert (MIDI::JACK_MidiPort::is_process_thread());
#endif // WITH_JACK_MIDI
#ifdef DEBUG_MIDI_CLOCK	
	cerr << "Tick with offset " << offset << endl;
#endif // DEBUG_MIDI_CLOCK
	static uint8_t _midi_clock_tick[1] = { MIDI_CMD_COMMON_CLOCK };
	_midi_port->write(_midi_clock_tick, 1, offset);
}

void MidiClockTicker::send_start_event(nframes_t offset)
{
	static uint8_t _midi_clock_tick[1] = { MIDI_CMD_COMMON_START };
	_midi_port->write(_midi_clock_tick, 1, offset);
}

void MidiClockTicker::send_continue_event(nframes_t offset)
{
	static uint8_t _midi_clock_tick[1] = { MIDI_CMD_COMMON_CONTINUE };
	_midi_port->write(_midi_clock_tick, 1, offset);
}

void MidiClockTicker::send_stop_event(nframes_t offset)
{
	static uint8_t _midi_clock_tick[1] = { MIDI_CMD_COMMON_STOP };
	_midi_port->write(_midi_clock_tick, 1, offset);
}

}

