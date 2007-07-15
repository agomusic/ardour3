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

#ifndef __ardour_midi_source_h__
#define __ardour_midi_source_h__

#include <string>

#include <time.h>

#include <glibmm/thread.h>

#include <sigc++/signal.h>

#include <ardour/source.h>
#include <ardour/ardour.h>
#include <ardour/buffer.h>
#include <ardour/midi_model.h>
#include <pbd/stateful.h>
#include <pbd/xml++.h>

using std::string;

namespace ARDOUR {

class MidiRingBuffer;

/** Source for MIDI data */
class MidiSource : public Source
{
  public:
	MidiSource (Session& session, string name);
	MidiSource (Session& session, const XMLNode&);
	virtual ~MidiSource ();
	
	virtual nframes_t read (MidiRingBuffer& dst, nframes_t start, nframes_t cnt, nframes_t stamp_offset) const;
	virtual nframes_t write (MidiRingBuffer& src, nframes_t cnt);

	virtual void mark_for_remove() = 0;
	virtual void mark_streaming_write_completed () {}

	string captured_for() const { return _captured_for; }
	void   set_captured_for (string str) { _captured_for = str; }

	uint32_t read_data_count()  const { return _read_data_count; }
	uint32_t write_data_count() const { return _write_data_count; }

	static sigc::signal<void,MidiSource*> MidiSourceCreated;
	       
	// The MIDI equivalent to "peaks" (but complete data)
	mutable sigc::signal<void,boost::shared_ptr<MidiBuffer>,nframes_t,nframes_t> ViewDataRangeReady;
	
	XMLNode& get_state ();
	int set_state (const XMLNode&);

	virtual void load_model(bool lock=true, bool force_reload=false) = 0;
	virtual void destroy_model() = 0;

	MidiModel* model() { return _model; }

  protected:
	virtual nframes_t read_unlocked (MidiRingBuffer& dst, nframes_t start, nframes_t cnt, nframes_t stamp_offset) const = 0;
	virtual nframes_t write_unlocked (MidiRingBuffer& dst, nframes_t cnt) = 0;
	
	mutable Glib::Mutex _lock;
	string              _captured_for;
	mutable uint32_t    _read_data_count;  ///< modified in read()
	mutable uint32_t    _write_data_count; ///< modified in write()

	MidiModel* _model;
	bool       _model_loaded;

  private:
	bool file_changed (string path);
};

}

#endif /* __ardour_midi_source_h__ */
