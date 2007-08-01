/*
    Copyright (C) 2006 Paul Davis
	Written by Dave Robillard, 2006

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

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <float.h>
#include <cerrno>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <algorithm>

#include <pbd/xml++.h>
#include <pbd/pthread_utils.h>

#include <ardour/midi_source.h>
#include <ardour/midi_ring_buffer.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

sigc::signal<void,MidiSource *> MidiSource::MidiSourceCreated;

MidiSource::MidiSource (Session& s, string name)
	: Source (s, name, DataType::MIDI)
	, _model(new MidiModel(s))
	, _model_loaded (false)
	, _writing (false)
{
	_read_data_count = 0;
	_write_data_count = 0;
}

MidiSource::MidiSource (Session& s, const XMLNode& node) 
	: Source (s, node)
	, _model(new MidiModel(s))
	, _model_loaded (false)
	, _writing (false)
{
	_read_data_count = 0;
	_write_data_count = 0;

	if (set_state (node)) {
		throw failed_constructor();
	}
}

MidiSource::~MidiSource ()
{
}

XMLNode&
MidiSource::get_state ()
{
	XMLNode& node (Source::get_state());

	if (_captured_for.length()) {
		node.add_property ("captured-for", _captured_for);
	}

	return node;
}

int
MidiSource::set_state (const XMLNode& node)
{
	const XMLProperty* prop;

	Source::set_state (node);

	if ((prop = node.property ("captured-for")) != 0) {
		_captured_for = prop->value();
	}

	return 0;
}

nframes_t
MidiSource::read (MidiRingBuffer& dst, nframes_t start, nframes_t cnt, nframes_t stamp_offset) const
{
	Glib::Mutex::Lock lm (_lock);
	if (_model_loaded && _model) {
		/*const size_t n_events = */_model->read(dst, start, cnt, stamp_offset);
		//cout << "Read " << n_events << " events from model." << endl;
		return cnt;
	} else {
		return read_unlocked (dst, start, cnt, stamp_offset);
	}
}

nframes_t
MidiSource::write (MidiRingBuffer& dst, nframes_t cnt)
{
	Glib::Mutex::Lock lm (_lock);
	return write_unlocked (dst, cnt);
}

bool
MidiSource::file_changed (string path)
{
	struct stat stat_file;

	int e1 = stat (path.c_str(), &stat_file);
	
	return ( !e1 );
}

void
MidiSource::mark_streaming_midi_write_started (NoteMode mode)
{
	if (_model) {
		_model->set_note_mode(mode);
		_model->start_write();
	}
	
	_writing = true;
}

void
MidiSource::mark_streaming_write_started ()
{
	if (_model)
		_model->start_write();

	_writing = true;
}

void
MidiSource::mark_streaming_write_completed ()
{
	if (_model)
		_model->end_write(false); // FIXME: param?

	_writing = false;
}

