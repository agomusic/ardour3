/*
    Copyright (C) 2006 Paul Davis 
    Author: Dave Robillard
    
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

#ifndef __ardour_midi_buffer_h__
#define __ardour_midi_buffer_h__

#include <midi++/event.h>
#include <ardour/buffer.h>

namespace ARDOUR {


/** Buffer containing 8-bit unsigned char (MIDI) data. */
class MidiBuffer : public Buffer
{
public:
	MidiBuffer(size_t capacity);
	~MidiBuffer();

	void silence(nframes_t dur, nframes_t offset=0);
	
	void read_from(const Buffer& src, nframes_t nframes, nframes_t offset);
	
	void copy(const MidiBuffer& copy);

	bool     push_back(const Evoral::MIDIEvent& event);
	bool     push_back(const jack_midi_event_t& event);
	uint8_t* reserve(double time, size_t size);

	void resize(size_t);

	bool merge(const MidiBuffer& a, const MidiBuffer& b);
	bool merge_in_place( const MidiBuffer &other );
	
	struct iterator {
		iterator(MidiBuffer& b, size_t i) : buffer(b), index(i) {}

		inline Evoral::MIDIEvent& operator*() const { return buffer[index]; }
		inline iterator& operator++() { ++index; return *this; } // prefix
		inline bool operator!=(const iterator& other) const { return index != other.index; }
		
		MidiBuffer& buffer;
		size_t      index;
	};
	
	struct const_iterator {
		const_iterator(const MidiBuffer& b, size_t i) : buffer(b), index(i) {}

		inline const Evoral::MIDIEvent& operator*() const { return buffer[index]; }
		inline const_iterator& operator++() { ++index; return *this; } // prefix
		inline bool operator!=(const const_iterator& other) const { return index != other.index; }
		
		const MidiBuffer& buffer;
		size_t            index;
	};

	iterator begin() { return iterator(*this, 0); }
	iterator end()   { return iterator(*this, _size); }

	const_iterator begin() const { return const_iterator(*this, 0); }
	const_iterator end()   const { return const_iterator(*this, _size); }

private:

	friend class iterator;
	friend class const_iterator;
	
	const Evoral::MIDIEvent& operator[](size_t i) const { assert(i < _size); return _events[i]; }
	Evoral::MIDIEvent& operator[](size_t i) { assert(i < _size); return _events[i]; }

	// FIXME: Eliminate this
	static const size_t MAX_EVENT_SIZE = 4; // bytes
	
	/* We use _size as "number of events", so the size of _data is
	 * (_size * MAX_EVENT_SIZE)
	 */

	/* FIXME: this is utter crap.  rewrite as a flat/packed buffer like MidiRingBuffer */

	Evoral::MIDIEvent* _events; ///< Event structs that point to offsets in _data
	uint8_t*           _data;   ///< MIDI, straight up.  No time stamps.
};


} // namespace ARDOUR

#endif // __ardour_midi_buffer_h__
