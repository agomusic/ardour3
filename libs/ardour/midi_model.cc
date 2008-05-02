/*
    Copyright (C) 2007 Paul Davis 
	Written by Dave Robillard, 2007

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

#define __STDC_LIMIT_MACROS 1

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <stdint.h>
#include <pbd/enumwriter.h>
#include <midi++/events.h>

#include <ardour/midi_model.h>
#include <ardour/midi_source.h>
#include <ardour/types.h>
#include <ardour/session.h>

using namespace std;
using namespace ARDOUR;


void 
MidiModel::write_lock()        
{ 
	_lock.writer_lock(); 
	_automation_lock.lock(); 
}

void 
MidiModel::write_unlock()      
{ 
	_lock.writer_unlock(); 
	_automation_lock.unlock(); 
}

void 
MidiModel::read_lock()   const 
{ 
	_lock.reader_lock(); 
	/*_automation_lock.lock();*/ 
}

void 
MidiModel::read_unlock() const 
{ 
	_lock.reader_unlock(); 
	/*_automation_lock.unlock();*/ 
}

// Read iterator (const_iterator)

MidiModel::const_iterator::const_iterator(const MidiModel& model, double t)
	: _model(&model)
	, _is_end( (t == DBL_MAX) || model.empty())
	, _locked( ! _is_end)
{
	//cerr << "Created MIDI iterator @ " << t << " (is end: " << _is_end << ")" << endl;
	
	if (_is_end)
		return;

	model.read_lock();
	
	_note_iter = model.notes().end();
	// find first note which begins after t
	for (MidiModel::Notes::const_iterator i = model.notes().begin(); i != model.notes().end(); ++i) {
		if ((*i)->time() >= t) {
			_note_iter = i;
			break;
		}
	}
			
	MidiControlIterator earliest_control(boost::shared_ptr<AutomationList>(), DBL_MAX, 0.0);

	_control_iters.reserve(model.controls().size());
	for (Automatable::Controls::const_iterator i = model.controls().begin();
			i != model.controls().end(); ++i) {

		assert(
			i->first.type() == MidiCCAutomation ||
			i->first.type() == MidiPgmChangeAutomation ||
			i->first.type() == MidiPitchBenderAutomation ||
			i->first.type() == MidiChannelAftertouchAutomation
		);

		double x, y;
		bool ret = i->second->list()->rt_safe_earliest_event_unlocked(t, DBL_MAX, x, y);
		if (!ret) {
			//cerr << "MIDI Iterator: CC " << i->first.id() << " (size " << i->second->list()->size()
			//	<< ") has no events past " << t << endl;
			continue;
		} 

		assert(x >= 0);
		assert(y >= i->first.min());
		assert(y <= i->first.max());
		
		const MidiControlIterator new_iter(i->second->list(), x, y);
		
		//cerr << "MIDI Iterator: CC " << i->first.id() << " added (" << x << ", " << y << ")" << endl;
		_control_iters.push_back(new_iter);

		if (x < earliest_control.x) {
			earliest_control = new_iter;
			_control_iter = _control_iters.end();
			--_control_iter;
		}
	}

	if (_note_iter != model.notes().end()) {
		_event = MIDI::Event((*_note_iter)->on_event(), false);
		_active_notes.push(*_note_iter);
		//cerr << " new const iterator: size active notes: " << _active_notes.size() << " is empty: " << _active_notes.empty() << endl;
		++_note_iter;
	}

	if (earliest_control.automation_list && earliest_control.x < _event.time())
		model.control_to_midi_event(_event, earliest_control);
	else
		_control_iter = _control_iters.end();
	
	if (_event.size() == 0) {
		//cerr << "Created MIDI iterator @ " << t << " is at end." << endl;
		_is_end = true;
		
		// FIXME: possible race condition here....
		if(_locked) {
			_model->read_unlock();
			_locked = false;
		}
	} else {
		printf("MIDI Iterator = %X @ %lf\n", _event.type(), _event.time());
	}
}


MidiModel::const_iterator::~const_iterator()
{
	if (_locked) {
		_model->read_unlock();
}
}

const MidiModel::const_iterator&
MidiModel::const_iterator::operator++()
{
	if (_is_end)
		throw std::logic_error("Attempt to iterate past end of MidiModel");

	/*cerr << "const_iterator::operator++: _event type:" << hex << "0x" << int(_event.type()) 
	     << "   buffer: 0x" << int(_event.buffer()[0]) << " 0x" << int(_event.buffer()[1]) 
	     << " 0x" << int(_event.buffer()[2]) << endl;*/

	if(! (_event.is_note() || _event.is_cc() || _event.is_pgm_change() || _event.is_pitch_bender() || _event.is_channel_aftertouch()) ) {
		cerr << "FAILED event buffer: " << hex << int(_event.buffer()[0]) << int(_event.buffer()[1]) << int(_event.buffer()[2]) << endl;
	}
	assert((_event.is_note() || _event.is_cc() || _event.is_pgm_change() || _event.is_pitch_bender() || _event.is_channel_aftertouch()));

	// TODO: This code crashes at the marked section
	// Increment past current control event
	if (_control_iter != _control_iters.end() && _control_iter->automation_list && _event.is_cc()) {
		double x, y;
		cerr << "control_iter x:" << _control_iter->x << " y:" << _control_iter->y << endl;
		// v--- this crashes because of a null pointer in the stl containers linked list chain
		//      the crash occurs in _control_iter->automation_list->size();
		const bool ret = _control_iter->automation_list->rt_safe_earliest_event_unlocked(
		                     _control_iter->x, DBL_MAX, x, y, false);

		if (ret) {
			//cerr << "Incremented " << _control_iter->automation_list->parameter().id() << " to " << x << endl;
			_control_iter->x = x;
			_control_iter->y = y;
		} else {
			//cerr << "Hit end of " << _control_iter->automation_list->parameter().id() << endl;
			_control_iter->automation_list.reset();
			_control_iter->x = DBL_MAX;
		}
	}

	// Now find and point at the earliest event


	const std::vector<MidiControlIterator>::iterator old_control_iter = _control_iter;
	_control_iter = _control_iters.begin();

	for (std::vector<MidiControlIterator>::iterator i = _control_iters.begin();
			i != _control_iters.end(); ++i) {
		if (i->x < _control_iter->x) {
			_control_iter = i;
		}
	}

	enum Type { NIL, NOTE_ON, NOTE_OFF, AUTOMATION };
	
	Type   type = NIL;
	double t    = 0;

	// Next earliest note on
	if (_note_iter != _model->notes().end()) {
		type = NOTE_ON;
		t = (*_note_iter)->time();
	}
	
	// Use the next earliest note off iff it's earlier than the note on
	if (_model->note_mode() == Sustained && (! _active_notes.empty())) {
		if (type == NIL || _active_notes.top()->end_time() <= (*_note_iter)->time()) {
			type = NOTE_OFF;
			t = _active_notes.top()->end_time();
		}
	}
	
	// Use the next earliest controller iff it's earlier than the note event
	if (_control_iter != _control_iters.end()
			&& _control_iter->x != DBL_MAX
			&& _control_iter != old_control_iter)
		if (type == NIL || _control_iter->x < t)
			type = AUTOMATION;

	if (type == NOTE_ON) {
		cerr << "********** MIDI Iterator = note on" << endl;
		_event = MIDI::Event((*_note_iter)->on_event(), false);
		_active_notes.push(*_note_iter);
		++_note_iter;
	} else if (type == NOTE_OFF) {
		cerr << "********** MIDI Iterator = note off" << endl;
		_event = MIDI::Event(_active_notes.top()->off_event(), false);
		_active_notes.pop();
	} else if (type == AUTOMATION) {
		cerr << "********** MIDI Iterator = AUTOMATION" << endl;
		_model->control_to_midi_event(_event, *_control_iter);
	} else {
		cerr << "********** MIDI Iterator = END" << endl;
		_is_end = true;
	}

	assert(_is_end || _event.size() > 0);

	return *this;
}
		

bool
MidiModel::const_iterator::operator==(const const_iterator& other) const
{
	if (_is_end || other._is_end)
		return (_is_end == other._is_end);
	else
		return (_event == other._event);
}
		

MidiModel::const_iterator&
MidiModel::const_iterator::operator=(const const_iterator& other)
{
	if (_locked && _model != other._model)
		_model->read_unlock();

	assert( ! other._event.owns_buffer());

	_model = other._model;
	_event = other._event;
	_active_notes = other._active_notes;
	_is_end = other._is_end;
	_locked = other._locked;
	_note_iter = other._note_iter;
	_control_iters = other._control_iters;
	_control_iter = other._control_iter;
	
	assert( ! _event.owns_buffer());
	
	return *this;
}

	
// MidiModel

MidiModel::MidiModel(MidiSource *s, size_t size)
	: Automatable(s->session(), "midi model")
	, _notes(size)
	, _note_mode(Sustained)
	, _writing(false)
	, _edited(false)
	, _end_iter(*this, DBL_MAX)
	, _next_read(UINT32_MAX)
	, _read_iter(*this, DBL_MAX)
	, _midi_source(s)
{
	assert(_end_iter._is_end);
	assert( ! _end_iter._locked);
}

/** Read events in frame range \a start .. \a start+cnt into \a dst,
 * adding \a stamp_offset to each event's timestamp.
 * \return number of events written to \a dst
 */
size_t
MidiModel::read(MidiRingBuffer& dst, nframes_t start, nframes_t nframes, nframes_t stamp_offset, nframes_t negative_stamp_offset) const
{
	//cerr << this << " MM::read @ " << start << " frames: " << nframes << " -> " << stamp_offset << endl;
	//cerr << this << " MM # notes: " << n_notes() << endl;

	size_t read_events = 0;

	if (start != _next_read) {
		_read_iter = const_iterator(*this, (double)start);
		//cerr << "Repositioning iterator from " << _next_read << " to " << start << endl;
	} else {
		//cerr << "Using cached iterator at " << _next_read << endl;
	}

	_next_read = start + nframes;

	while (_read_iter != end() && _read_iter->time() < start + nframes) {
		assert(_read_iter->size() > 0);
		dst.write(_read_iter->time() + stamp_offset - negative_stamp_offset, _read_iter->size(), _read_iter->buffer());
		
		/*
		cerr << this << " MidiModel::read event @ " << _read_iter->time()  
		     << " type: " << hex << int(_read_iter->type()) << dec 
		     << " note: " << int(_read_iter->note()) 
		     << " velocity: " << int(_read_iter->velocity()) 
		     << endl;
		*/
		
		++_read_iter;
		++read_events;
	}

	return read_events;
}
	

bool
MidiModel::control_to_midi_event(MIDI::Event& ev, const MidiControlIterator& iter) const
{
	switch(iter.automation_list->parameter().type()) {
	case MidiCCAutomation:
		if (ev.size() < 3)
			ev.set_buffer((Byte*)malloc(3), true);

		assert(iter.automation_list);
		assert(iter.automation_list->parameter().channel() < 16);
		assert(iter.automation_list->parameter().id() <= INT8_MAX);
		assert(iter.y <= INT8_MAX);
		ev.buffer()[0] = MIDI_CMD_CONTROL + iter.automation_list->parameter().channel();
		ev.buffer()[1] = (Byte)iter.automation_list->parameter().id();
		ev.buffer()[2] = (Byte)iter.y;
		ev.time() = iter.x;
		ev.size() = 3;
		return true;
		
	case MidiPgmChangeAutomation:
		if (ev.size() < 3)
			ev.set_buffer((Byte*)malloc(3), true);

		assert(iter.automation_list);
		assert(iter.automation_list->parameter().channel() < 16);
		assert(iter.automation_list->parameter().id() <= INT8_MAX);
		assert(iter.y <= INT8_MAX);
		ev.buffer()[0] = MIDI_CMD_PGM_CHANGE + iter.automation_list->parameter().channel();
		ev.buffer()[1] = (Byte)iter.y;
		ev.buffer()[2] = 0;
		ev.time() = iter.x;
		ev.size() = 3;
		return true;
		
	case MidiPitchBenderAutomation:
		if (ev.size() < 3)
			ev.set_buffer((Byte*)malloc(3), true);

		assert(iter.automation_list);
		assert(iter.automation_list->parameter().channel() < 16);
		assert(iter.automation_list->parameter().id() <= INT8_MAX);
		assert(iter.y < (1<<14));
		ev.buffer()[0] = MIDI_CMD_BENDER + iter.automation_list->parameter().channel();
		ev.buffer()[1] = ((Byte)iter.y) & 0x7F; // LSB
		ev.buffer()[2] = (((Byte)iter.y) >> 7) & 0x7F; // MSB
		ev.time() = iter.x;
		ev.size() = 3;
		return true;		

	case MidiChannelAftertouchAutomation:
		if (ev.size() < 3)
			ev.set_buffer((Byte*)malloc(3), true);

		assert(iter.automation_list);
		assert(iter.automation_list->parameter().channel() < 16);
		assert(iter.automation_list->parameter().id() <= INT8_MAX);
		assert(iter.y <= INT8_MAX);
		ev.buffer()[0] = MIDI_CMD_CHANNEL_PRESSURE + iter.automation_list->parameter().channel();
		ev.buffer()[1] = (Byte)iter.y;
		ev.buffer()[2] = 0;
		ev.time() = iter.x;
		ev.size() = 3;
		return true;
		
	default:
		return false;
	}
}

	
/** Begin a write of events to the model.
 *
 * If \a mode is Sustained, complete notes with duration are constructed as note
 * on/off events are received.  Otherwise (Percussive), only note on events are
 * stored; note off events are discarded entirely and all contained notes will
 * have duration 0.
 */
void
MidiModel::start_write()
{
	//cerr << "MM " << this << " START WRITE, MODE = " << enum_2_string(_note_mode) << endl;
	write_lock();
	_writing = true;
	for (int i = 0; i < 16; ++i)
		_write_notes[i].clear();
	write_unlock();
}



/** Finish a write of events to the model.
 *
 * If \a delete_stuck is true and the current mode is Sustained, note on events
 * that were never resolved with a corresonding note off will be deleted.
 * Otherwise they will remain as notes with duration 0.
 */
void
MidiModel::end_write(bool delete_stuck)
{
	write_lock();
	assert(_writing);
	
	//cerr << "MM " << this << " END WRITE: " << _notes.size() << " NOTES\n";

	if (_note_mode == Sustained && delete_stuck) {
		for (Notes::iterator n = _notes.begin(); n != _notes.end() ; ) {
			if ((*n)->duration() == 0) {
				cerr << "WARNING: Stuck note lost: " << (*n)->note() << endl;
				n = _notes.erase(n);
				// we have to break here because erase invalidates the iterator
				break;
			} else {
				++n;
			}
		}
	}

	for (int i = 0; i < 16; ++i) {
		if (!_write_notes[i].empty()) {
			cerr << "WARNING: MidiModel::end_write: Channel " << i << " has "
					<< _write_notes[i].size() << " stuck notes" << endl;
		}
		_write_notes[i].clear();
	}
	
	_writing = false;
	write_unlock();
}


/** Append \a in_event to model.  NOT realtime safe.
 *
 * Timestamps of events in \a buf are expected to be relative to
 * the start of this model (t=0) and MUST be monotonically increasing
 * and MUST be >= the latest event currently in the model.
 */
void
MidiModel::append(const MIDI::Event& ev)
{
	write_lock();
	_edited = true;
	
	cerr << "MidiModel::append event type: " << hex << "0x" << int(ev.type()) << endl;

	assert(_notes.empty() || ev.time() >= _notes.back()->time());
	assert(_writing);

	if (ev.is_note_on()) {
		append_note_on_unlocked(ev.channel(), ev.time(), ev.note(), ev.velocity());
	} else if (ev.is_note_off()) {
		append_note_off_unlocked(ev.channel(), ev.time(), ev.note());
	} else if (ev.is_cc()) {
		append_automation_event_unlocked(MidiCCAutomation, ev.channel(), ev.time(), ev.cc_number(), ev.cc_value());
	} else if (ev.is_pgm_change()) {
		append_automation_event_unlocked(MidiPgmChangeAutomation, ev.channel(), ev.time(), ev.pgm_number(), 0);
	} else if (ev.is_pitch_bender()) {
		append_automation_event_unlocked(MidiPitchBenderAutomation, ev.channel(), ev.time(), ev.pitch_bender_lsb(), ev.pitch_bender_msb());
	} else if (ev.is_channel_aftertouch()) {
		append_automation_event_unlocked(MidiChannelAftertouchAutomation, ev.channel(), ev.time(), ev.channel_aftertouch(), 0);
	} else { 
		printf("MM Unknown event type %X\n", ev.type());
	}
	
	write_unlock();
}


void
MidiModel::append_note_on_unlocked(uint8_t chan, double time, uint8_t note_num, uint8_t velocity)
{
	/*cerr << "MidiModel " << this << " chan " << (int)chan <<
			" note " << (int)note_num << " on @ " << time << endl;*/

	assert(chan < 16);
	assert(_writing);
	_edited = true;

	_notes.push_back(boost::shared_ptr<Note>(new Note(chan, time, 0, note_num, velocity)));
	if (_note_mode == Sustained) {
		//cerr << "MM Sustained: Appending active note on " << (unsigned)(uint8_t)note_num << endl;
		_write_notes[chan].push_back(_notes.size() - 1);
	}/* else {
		cerr << "MM Percussive: NOT appending active note on" << endl;
	}*/
}


void
MidiModel::append_note_off_unlocked(uint8_t chan, double time, uint8_t note_num)
{
	/*cerr << "MidiModel " << this << " chan " << (int)chan <<
			" note " << (int)note_num << " off @ " << time << endl;*/

	assert(chan < 16);
	assert(_writing);
	_edited = true;

	if (_note_mode == Percussive) {
		cerr << "MidiModel Ignoring note off (percussive mode)" << endl;
		return;
	}

	/* FIXME: make _write_notes fixed size (127 noted) for speed */
	
	/* FIXME: note off velocity for that one guy out there who actually has
	 * keys that send it */

	bool resolved = false;

	for (WriteNotes::iterator n = _write_notes[chan].begin(); n != _write_notes[chan].end(); ++n) {
		Note& note = *_notes[*n].get();
		//cerr << (unsigned)(uint8_t)note.note() << " ? " << (unsigned)note_num << endl;
		if (note.note() == note_num) {
			assert(time >= note.time());
			note.set_duration(time - note.time());
			_write_notes[chan].erase(n);
			//cerr << "MM resolved note, duration: " << note.duration() << endl;
			resolved = true;
			break;
		}
	}

	if (!resolved)
		cerr << "MidiModel " << this << " spurious note off chan " << (int)chan
			<< ", note " << (int)note_num << " @ " << time << endl;
}


void
MidiModel::append_automation_event_unlocked(AutomationType type, uint8_t chan, double time, uint8_t first_byte, uint8_t second_byte)
{
	//cerr << "MidiModel " << this << " chan " << (int)chan <<
	//		" CC " << (int)number << " = " << (int)value << " @ " << time << endl;
	
	assert(chan < 16);
	assert(_writing);
	_edited = true;
	double value;
	
	uint32_t id = 0;
	
	switch(type) {
	case MidiCCAutomation:
		id = first_byte;
		value = double(second_byte);
		break;
	case MidiChannelAftertouchAutomation:
	case MidiPgmChangeAutomation:
		id = 0;
		value = double(first_byte);
		break;
	case MidiPitchBenderAutomation:
		id = 0;
		value = double((0x7F & second_byte) << 7 | (0x7F & first_byte));
		break;		
	default:
		assert(false);
	}
	
	Parameter param(type, id, chan);
	boost::shared_ptr<AutomationControl> control = Automatable::control(param, true);
	control->list()->fast_simple_add(time, value);
}

void
MidiModel::add_note_unlocked(const boost::shared_ptr<Note> note)
{
	//cerr << "MidiModel " << this << " add note " << (int)note.note() << " @ " << note.time() << endl;
	_edited = true;
	Notes::iterator i = upper_bound(_notes.begin(), _notes.end(), note, note_time_comparator);
	_notes.insert(i, note);
}


void
MidiModel::remove_note_unlocked(const boost::shared_ptr<const Note> note)
{
	_edited = true;
	//cerr << "MidiModel " << this << " remove note " << (int)note.note() << " @ " << note.time() << endl;
	for(Notes::iterator n = _notes.begin(); n != _notes.end(); ++n) {
		Note& _n = *(*n);
		const Note& _note = *note;
		// TODO: There is still the issue, that after restarting ardour
		// persisted undo does not work, because of rounding errors in the
		// event times after saving/restoring to/from MIDI files
		cerr << "======================================= " << endl;
		cerr << int(_n.note()) << "@" << int(_n.time()) << "[" << int(_n.channel()) << "] --" << int(_n.duration()) << "-- #" << int(_n.velocity())  << endl;
		cerr << int(_note.note()) << "@" << int(_note.time()) << "[" << int(_note.channel()) << "] --" << int(_note.duration()) << "-- #" << int(_note.velocity())  << endl;
		cerr << "Equal: " << bool(_n == _note) << endl;
		cerr << endl << endl;
		if(_n == _note) {
			_notes.erase(n);
			// we have to break here, because erase invalidates all iterators, ie. n itself
			break;
		}		
	}
}

/** Slow!  for debugging only. */
#ifndef NDEBUG
bool
MidiModel::is_sorted() const
{
	bool t = 0;
	for (Notes::const_iterator n = _notes.begin(); n != _notes.end(); ++n)
		if ((*n)->time() < t)
			return false;
		else
			t = (*n)->time();

	return true;
}
#endif

/** Start a new command.
 *
 * This has no side-effects on the model or Session, the returned command
 * can be held on to for as long as the caller wishes, or discarded without
 * formality, until apply_command is called and ownership is taken.
 */
MidiModel::DeltaCommand*
MidiModel::new_delta_command(const string name)
{
	DeltaCommand* cmd =  new DeltaCommand(_midi_source->model(), name);
	return cmd;
}


/** Apply a command.
 *
 * Ownership of cmd is taken, it must not be deleted by the caller.
 * The command will constitute one item on the undo stack.
 */
void
MidiModel::apply_command(Command* cmd)
{
	_session.begin_reversible_command(cmd->name());
	(*cmd)();
	assert(is_sorted());
	_session.commit_reversible_command(cmd);
	_edited = true;
}


// MidiEditCommand

MidiModel::DeltaCommand::DeltaCommand(boost::shared_ptr<MidiModel> m, const std::string& name)
	: Command(name), _model(m), _name(name) 
{
	
}

MidiModel::DeltaCommand::DeltaCommand(boost::shared_ptr<MidiModel> m, const XMLNode& node)
	: _model(m)
{
	set_state(node);
}

void
MidiModel::DeltaCommand::add(const boost::shared_ptr<Note> note)
{
	//cerr << "MEC: apply" << endl;

	_removed_notes.remove(note);
	_added_notes.push_back(note);
}


void
MidiModel::DeltaCommand::remove(const boost::shared_ptr<Note> note)
{
	//cerr << "MEC: remove" << endl;

	_added_notes.remove(note);
	_removed_notes.push_back(note);
}

		
void 
MidiModel::DeltaCommand::operator()()
{
	// This could be made much faster by using a priority_queue for added and
	// removed notes (or sort here), and doing a single iteration over _model
	
	// Need to reset iterator to drop the read lock it holds, or we'll deadlock
	const bool   reset_iter = (_model->_read_iter.locked());
	const double iter_time  = _model->_read_iter->time();

	if (reset_iter)
		_model->_read_iter = _model->end(); // drop read lock

	assert( ! _model->_read_iter.locked());

	_model->write_lock();
	
	for (std::list< boost::shared_ptr<Note> >::iterator i = _added_notes.begin(); i != _added_notes.end(); ++i)
		_model->add_note_unlocked(*i);
	
	for (std::list< boost::shared_ptr<Note> >::iterator i = _removed_notes.begin(); i != _removed_notes.end(); ++i)
		_model->remove_note_unlocked(*i);
	
	_model->write_unlock();

	if (reset_iter)
		_model->_read_iter = const_iterator(*_model.get(), iter_time);
	
	_model->ContentsChanged(); /* EMIT SIGNAL */
}


void
MidiModel::DeltaCommand::undo()
{
	// This could be made much faster by using a priority_queue for added and
	// removed notes (or sort here), and doing a single iteration over _model
	
	// Need to reset iterator to drop the read lock it holds, or we'll deadlock
	const bool   reset_iter = (_model->_read_iter.locked());
	const double iter_time  = _model->_read_iter->time();

	if (reset_iter)
		_model->_read_iter = _model->end(); // drop read lock
	
	assert( ! _model->_read_iter.locked());

	_model->write_lock();

	for (std::list< boost::shared_ptr<Note> >::iterator i = _added_notes.begin(); i != _added_notes.end(); ++i)
		_model->remove_note_unlocked(*i);
	
	for (std::list< boost::shared_ptr<Note> >::iterator i = _removed_notes.begin(); i != _removed_notes.end(); ++i)
		_model->add_note_unlocked(*i);
	
	_model->write_unlock();
	
	if (reset_iter)
		_model->_read_iter = const_iterator(*_model.get(), iter_time);
	
	_model->ContentsChanged(); /* EMIT SIGNAL */
}

XMLNode &
MidiModel::DeltaCommand::marshal_note(const boost::shared_ptr<Note> note)
{
	XMLNode *xml_note = new XMLNode("note");
	ostringstream note_str(ios::ate);
	note_str << int(note->note());
	xml_note->add_property("note", note_str.str());

	ostringstream channel_str(ios::ate);
	channel_str << int(note->channel());
	xml_note->add_property("channel", channel_str.str());	
	
	ostringstream time_str(ios::ate);
	time_str << int(note->time());
	xml_note->add_property("time", time_str.str());

	ostringstream duration_str(ios::ate);
	duration_str <<(unsigned int) note->duration();
	xml_note->add_property("duration", duration_str.str());

	ostringstream velocity_str(ios::ate);
	velocity_str << (unsigned int) note->velocity();
	xml_note->add_property("velocity", velocity_str.str());
	
	return *xml_note;
}

boost::shared_ptr<Note> 
MidiModel::DeltaCommand::unmarshal_note(XMLNode *xml_note) 
{
	unsigned int note;
	istringstream note_str(xml_note->property("note")->value());
	note_str >> note;

	unsigned int channel;
	istringstream channel_str(xml_note->property("channel")->value());
	channel_str >> channel;

	unsigned int time;
	istringstream time_str(xml_note->property("time")->value());
	time_str >> time;

	unsigned int duration;
	istringstream duration_str(xml_note->property("duration")->value());
	duration_str >> duration;

	unsigned int velocity;
	istringstream velocity_str(xml_note->property("velocity")->value());
	velocity_str >> velocity;
	
	boost::shared_ptr<Note> note_ptr(new Note(channel, time, duration, note, velocity));
	return note_ptr;
}

#define ADDED_NOTES_ELEMENT "added_notes"
#define REMOVED_NOTES_ELEMENT "removed_notes"
#define DELTA_COMMAND_ELEMENT "DeltaCommand"

int 
MidiModel::DeltaCommand::set_state (const XMLNode& delta_command)
{
	if(delta_command.name() != string(DELTA_COMMAND_ELEMENT)) {
		return 1;
	}
	
	_added_notes.clear();
	XMLNode *added_notes = delta_command.child(ADDED_NOTES_ELEMENT);
	XMLNodeList notes = added_notes->children();
	transform(notes.begin(), notes.end(), back_inserter(_added_notes), 
	          sigc::mem_fun(*this, &DeltaCommand::unmarshal_note));
	
	_removed_notes.clear();
	XMLNode *removed_notes = delta_command.child(REMOVED_NOTES_ELEMENT);
	notes = removed_notes->children();
	transform(notes.begin(), notes.end(), back_inserter(_removed_notes), 
			sigc::mem_fun(*this, &DeltaCommand::unmarshal_note));
	
	return 0;
}

XMLNode& 
MidiModel::DeltaCommand::get_state () 
{
	XMLNode *delta_command = new XMLNode(DELTA_COMMAND_ELEMENT);
	delta_command->add_property("midi_source", _model->midi_source()->id().to_s());
	
	XMLNode *added_notes   = delta_command->add_child(ADDED_NOTES_ELEMENT);
	for_each(_added_notes.begin(), _added_notes.end(), 
		sigc::compose(sigc::mem_fun(*added_notes, &XMLNode::add_child_nocopy),
		              sigc::mem_fun(*this, &DeltaCommand::marshal_note))); 
	
	XMLNode *removed_notes   = delta_command->add_child(REMOVED_NOTES_ELEMENT);
	for_each(_removed_notes.begin(), _removed_notes.end(), 
		sigc::compose(sigc::mem_fun(*removed_notes, &XMLNode::add_child_nocopy),
		              sigc::mem_fun(*this, &DeltaCommand::marshal_note))); 

	return *delta_command;
}


struct EventTimeComparator {
	typedef const MIDI::Event* value_type;
	inline bool operator()(const MIDI::Event* a,
	                       const MIDI::Event* b) const { 
		return a->time() >= b->time();
	}
};

bool
MidiModel::write_to(boost::shared_ptr<MidiSource> source)
{
	cerr << "Writing model to " << source->name() << endl;

	/* This could be done using a temporary MidiRingBuffer and using
	 * MidiModel::read and MidiSource::write, but this is more efficient
	 * and doesn't require any buffer size assumptions (ie it's worth
	 * the code duplication).
	 *
	 * This is also different from read in that note off events are written
	 * regardless of the track mode.  This is so the user can switch a
	 * recorded track (with note durations from some instrument) to percussive,
	 * save, reload, then switch it back to sustained preserving the original
	 * note durations.
	 */

	read_lock();

	LaterNoteEndComparator cmp;
	ActiveNotes active_notes(cmp);
	
	EventTimeComparator comp;
	typedef std::priority_queue<
				const MIDI::Event*, 
				std::deque<const MIDI::Event*>,
				EventTimeComparator> MidiEvents;
	
	MidiEvents events(comp);
	
	/* Why sort manyally, when a priority queue does the job for us,
	 * (I am probably wrong here, but I needed that to test program
	 * change code quickly) ???
	 * 	*/
	// Foreach note
	for (Notes::const_iterator n = _notes.begin(); n != _notes.end(); ++n) {

		// Write any pending note offs earlier than this note on
		while ( ! active_notes.empty() ) {
			const boost::shared_ptr<const Note> earliest_off = active_notes.top();
			const MIDI::Event& off_ev = earliest_off->off_event();
			if (off_ev.time() <= (*n)->time()) {
				events.push(&off_ev);
				active_notes.pop();
			} else {
				break;
			}
		}

		// Write this note on
		events.push(&(*n)->on_event());
		if ((*n)->duration() > 0)
			active_notes.push(*n);
	}
		
	// Write any trailing note offs
	while ( ! active_notes.empty() ) {
		events.push(&active_notes.top()->off_event());
		active_notes.pop();
	}
	
	while(!events.empty()) {
		source->append_event_unlocked(Frames, *events.top());
		cerr << "MidiModel::write_to appending event with time:" << dec << int(events.top()->time()) << hex 
		     << "   buffer: 0x" << int(events.top()->buffer()[0]) << " 0x" << int(events.top()->buffer()[1]) 
		     << " 0x" << int(events.top()->buffer()[2]) << endl;
		events.pop();
	}
	
	_edited = false;
	
	read_unlock();

	return true;
}

XMLNode&
MidiModel::get_state()
{
	XMLNode *node = new XMLNode("MidiModel");
	return *node;
}

const MidiSource * 
MidiModel::midi_source() const
{ 
	return _midi_source; 
}

void 
MidiModel::set_midi_source(MidiSource *source) 
{ 
	_midi_source = source; 
} 
