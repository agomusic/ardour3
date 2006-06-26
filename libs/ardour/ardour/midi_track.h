/*
    Copyright (C) 2006 Paul Davis 
	Written by Dave Robillard
 
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

#ifndef __ardour_midi_track_h__
#define __ardour_midi_track_h__

#include <ardour/route.h>

namespace ARDOUR
{

class Session;
class MidiDiskstream;
class MidiPlaylist;
class RouteGroup;

class MidiTrack : public Route
{
public:
	MidiTrack (Session&, string name, Route::Flag f = Route::Flag (0), TrackMode m = Normal);
	MidiTrack (Session&, const XMLNode&);
	~MidiTrack ();

	int set_name (string str, void *src);

	int  roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame,
	           jack_nframes_t offset, int declick, bool can_record, bool rec_monitors_input);

	int  no_roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame,
	              jack_nframes_t offset, bool state_changing, bool can_record, bool rec_monitors_input);

	int  silent_roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame,
	                  jack_nframes_t offset, bool can_record, bool rec_monitors_input);

	void toggle_monitor_input ();

	bool can_record() const { return true; }

	void set_record_enable (bool yn, void *src);

	MidiDiskstream& disk_stream() const { return *diskstream; }

	int set_diskstream (MidiDiskstream&, void *);
	int use_diskstream (string name);
	int use_diskstream (id_t id);

	TrackMode mode() const { return _mode; }

	void set_mode (TrackMode m);
	sigc::signal<void> ModeChanged;

	jack_nframes_t update_total_latency();
	void           set_latency_delay (jack_nframes_t);

	int export_stuff (vector<unsigned char*>& buffers, char * workbuf, uint32_t nbufs,
		jack_nframes_t nframes, jack_nframes_t end_frame);

	sigc::signal<void,void*> diskstream_changed;

	enum FreezeState {
	    NoFreeze,
	    Frozen,
	    UnFrozen
	};

	FreezeState freeze_state() const;

	sigc::signal<void> FreezeChange;

	void freeze (InterThreadInfo&);
	void unfreeze ();

	void bounce (InterThreadInfo&);
	void bounce_range (jack_nframes_t start, jack_nframes_t end, InterThreadInfo&);

	XMLNode& get_state();
	XMLNode& get_template();
	int set_state(const XMLNode& node);

	MIDI::Controllable& midi_rec_enable_control() { return _midi_rec_enable_control; }

	void reset_midi_control (MIDI::Port*, bool);
	void send_all_midi_feedback ();

	bool record_enabled() const;
	void set_meter_point (MeterPoint, void* src);

protected:
	MidiDiskstream *diskstream;
	MeterPoint _saved_meter_point;
	TrackMode _mode;

	XMLNode& state (bool full);

	void passthru_silence (jack_nframes_t start_frame, jack_nframes_t end_frame,
	                       jack_nframes_t nframes, jack_nframes_t offset, int declick,
	                       bool meter);

	uint32_t n_process_buffers ();

private:
	struct FreezeRecordInsertInfo
	{
		FreezeRecordInsertInfo(XMLNode& st)
				: state (st), insert (0)
		{}

		XMLNode    state;
		Insert*    insert;
		id_t       id;
		UndoAction memento;
	};

	struct FreezeRecord
	{
		FreezeRecord()
		{
			playlist = 0;
			have_mementos = false;
		}

		~FreezeRecord();

		MidiPlaylist* playlist;
		vector<FreezeRecordInsertInfo*> insert_info;
		bool have_mementos;
		FreezeState state;
	};

	FreezeRecord _freeze_record;
	XMLNode* pending_state;

	void diskstream_record_enable_changed (void *src);
	void diskstream_input_channel_changed (void *src);

	void input_change_handler (void *src);

	sigc::connection recenable_connection;
	sigc::connection ic_connection;

	int deprecated_use_diskstream_connections ();
	void set_state_part_two ();
	void set_state_part_three ();

	struct MIDIRecEnableControl : public MIDI::Controllable
	{
		MIDIRecEnableControl (MidiTrack&, MIDI::Port *);
		void set_value (float);
		void send_feedback (bool);
		MIDI::byte* write_feedback (MIDI::byte* buf, int32_t& bufsize, bool val, bool force = false);
		MidiTrack& track;
		bool setting;
		bool last_written;
	};

	MIDIRecEnableControl _midi_rec_enable_control;

	bool _destructive;
};

}
; /* namespace ARDOUR*/

#endif /* __ardour_midi_track_h__ */
