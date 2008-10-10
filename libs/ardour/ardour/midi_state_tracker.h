/*
    Copyright (C) 2008 Paul Davis 
    Author: Torben Hohn
    
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

#ifndef __ardour_midi_state_tracker_h__
#define __ardour_midi_state_tracker_h__

#include <bitset>

#include <ardour/midi_buffer.h>


namespace ARDOUR {


/** Tracks played notes, so they can be resolved in potential stuck note
 * situations (e.g. looping, transport stop, etc).
 */
class MidiStateTracker
{
public:
	MidiStateTracker();

	bool track (const MidiBuffer::iterator& from, const MidiBuffer::iterator& to);
	void resolve_notes (MidiBuffer& buffer, nframes_t time);

private:
	void track_note_onoffs(Evoral::MIDIEvent& event);

	std::bitset<128*16> _active_notes;
};


} // namespace ARDOUR

#endif // __ardour_midi_state_tracker_h__ 
