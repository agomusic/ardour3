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

    $Id: diskstream.h 579 2006-06-12 19:56:37Z essej $
*/

#ifndef __ardour_diskstream_h__
#define __ardour_diskstream_h__

#include <sigc++/signal.h>

#include <cmath>
#include <string>
#include <queue>
#include <map>
#include <vector>

#include <time.h>

#include <pbd/fastlog.h>
#include <pbd/ringbufferNPT.h>
#include <pbd/stateful.h>

#include <ardour/ardour.h>
#include <ardour/configuration.h>
#include <ardour/session.h>
#include <ardour/route_group.h>
#include <ardour/route.h>
#include <ardour/port.h>
#include <ardour/utils.h>


struct tm;

namespace ARDOUR {

class AudioEngine;
class Send;
class Session;
class Playlist;
class IO;

class Diskstream : public Stateful, public sigc::trackable
{	
  public:
	enum Flag {
		Recordable = 0x1,
		Hidden = 0x2,
		Destructive = 0x4
	};

	string      name () const { return _name; }
	virtual int set_name (string str);

	ARDOUR::IO* io() const { return _io; }
	void set_io (ARDOUR::IO& io);

	virtual Diskstream& ref() { _refcnt++; return *this; }
	void     unref() { if (_refcnt) _refcnt--; if (_refcnt == 0) delete this; }
	uint32_t refcnt() const { return _refcnt; }

	virtual float playback_buffer_load() const = 0;
	virtual float capture_buffer_load() const = 0;

	void set_flag (Flag f)   { _flags |= f; }
	void unset_flag (Flag f) { _flags &= ~f; }

	AlignStyle alignment_style() const { return _alignment_style; }
	void       set_align_style (AlignStyle);
	void       set_persistent_align_style (AlignStyle a) { _persistent_alignment_style = a; }
	
	jack_nframes_t roll_delay() const { return _roll_delay; }
	void           set_roll_delay (jack_nframes_t);

	bool         record_enabled() const { return g_atomic_int_get (&_record_enabled); }
	virtual void set_record_enabled (bool yn) = 0;

	bool destructive() const { return _flags & Destructive; }
	virtual void set_destructive (bool yn);

	const PBD::ID& id()          const { return _id; }
	bool           hidden()      const { return _flags & Hidden; }
	bool           recordable()  const { return _flags & Recordable; }
	bool           reversed()    const { return _actual_speed < 0.0f; }
	double         speed()       const { return _visible_speed; }
	
	virtual void punch_in()  {}
	virtual void punch_out() {}

	void set_speed (double);
	void non_realtime_set_speed ();

	Playlist* playlist () { return _playlist; }

	virtual int use_playlist (Playlist *);
	virtual int use_new_playlist () = 0;
	virtual int use_copy_playlist () = 0;

	jack_nframes_t current_capture_start() const { return capture_start_frame; }
	jack_nframes_t current_capture_end()   const { return capture_start_frame + capture_captured; }
	jack_nframes_t get_capture_start_frame (uint32_t n=0);
	jack_nframes_t get_captured_frames (uint32_t n=0);
	
	uint32_t n_channels() { return _n_channels; }

	static jack_nframes_t disk_io_frames()                      { return disk_io_chunk_frames; }
	static void           set_disk_io_chunk_frames (uint32_t n) { disk_io_chunk_frames = n; }

	/* Stateful */
	virtual XMLNode& get_state(void) = 0;
	virtual int      set_state(const XMLNode& node) = 0;
	
	// FIXME: makes sense for all diskstream types?
	virtual void monitor_input (bool) {}

	jack_nframes_t capture_offset() const { return _capture_offset; }
	virtual void   set_capture_offset ();

	bool slaved() const      { return _slaved; }
	void set_slaved(bool yn) { _slaved = yn; }

	int set_loop (Location *loc);

	std::list<Region*>& last_capture_regions () { return _last_capture_regions; }

	void handle_input_change (IOChange, void *src);

	sigc::signal<void>            RecordEnableChanged;
	sigc::signal<void>            SpeedChanged;
	sigc::signal<void>            ReverseChanged;
	sigc::signal<void>            PlaylistChanged;
	sigc::signal<void>            AlignmentStyleChanged;
	sigc::signal<void,Location *> LoopSet;

	static sigc::signal<void>                DiskOverrun;
	static sigc::signal<void>                DiskUnderrun;
	static sigc::signal<void,Diskstream*>    DiskstreamCreated; // XXX use a ref with sigc2
	static sigc::signal<void,list<Source*>*> DeleteSources;

  protected:
	friend class Session;

	Diskstream (Session &, const string& name, Flag f = Recordable);
	Diskstream (Session &, const XMLNode&);

	/* the Session is the only point of access for these because they require
	 * that the Session is "inactive" while they are called.
	 */

	virtual void set_pending_overwrite (bool) = 0;
	virtual int  overwrite_existing_buffers () = 0;
	virtual void set_block_size (jack_nframes_t) = 0;
	virtual int  internal_playback_seek (jack_nframes_t distance) = 0;
	virtual int  can_internal_playback_seek (jack_nframes_t distance) = 0;
	virtual int  rename_write_sources () = 0;
	virtual void reset_write_sources (bool, bool force = false) = 0;
	virtual void non_realtime_input_change () = 0;

	uint32_t read_data_count() const { return _read_data_count; }
	uint32_t write_data_count() const { return _write_data_count; }

  protected:
	friend class Auditioner;
	virtual int  seek (jack_nframes_t which_sample, bool complete_refill = false) = 0;

  protected:
	friend class Track;

	virtual void prepare ();
	virtual int  process (jack_nframes_t transport_frame, jack_nframes_t nframes, jack_nframes_t offset, bool can_record, bool rec_monitors_input) = 0;
	virtual bool commit  (jack_nframes_t nframes) = 0;
	virtual void recover (); /* called if commit will not be called, but process was */

	//private:
	
	/** Use unref() to destroy a diskstream */
	virtual ~Diskstream();

	enum TransitionType {
		CaptureStart = 0,
		CaptureEnd
	};
	
	struct CaptureTransition {
		TransitionType   type;
		jack_nframes_t   capture_val; ///< The start or end file frame position
	};

	/* The two central butler operations */
	virtual int do_flush (Session::RunContext context, bool force = false) = 0;
	virtual int do_refill () = 0;
	
	/** For non-butler contexts (allocates temporary working buffers) */
	virtual int do_refill_with_alloc() = 0;

	
	/* XXX fix this redundancy ... */

	virtual void playlist_changed (Change);
	virtual void playlist_modified ();
	virtual void playlist_deleted (Playlist*);

	virtual void finish_capture (bool rec_monitors_input) = 0;
	virtual void transport_stopped (struct tm&, time_t, bool abort) = 0;

	struct CaptureInfo {
	    uint32_t start;
	    uint32_t frames;
	};

	virtual void init (Flag);

	virtual int use_new_write_source (uint32_t n=0) = 0;

	virtual int find_and_use_playlist (const string&) = 0;

	virtual void allocate_temporary_buffers () = 0;

	virtual bool realtime_set_speed (double, bool global_change);

	std::list<Region*> _last_capture_regions;
	virtual int use_pending_capture_data (XMLNode& node) = 0;

	virtual void get_input_sources () = 0;
	virtual void check_record_status (jack_nframes_t transport_frame, jack_nframes_t nframes, bool can_record) = 0;
	virtual void set_align_style_from_io() {}
	virtual void setup_destructive_playlist () = 0;
	virtual void use_destructive_playlist () = 0;

	static jack_nframes_t disk_io_chunk_frames;
	vector<CaptureInfo*>  capture_info;
	Glib::Mutex           capture_info_lock;

	uint32_t i_am_the_modifier;

	string            _name;
	ARDOUR::Session&  _session;
	ARDOUR::IO*       _io;
	uint32_t          _n_channels;
	PBD::ID           _id;
	Playlist*         _playlist;

	mutable gint             _record_enabled;
	double                   _visible_speed;
	double                   _actual_speed;
	/* items needed for speed change logic */
	bool                     _buffer_reallocation_required;
	bool                     _seek_required;
	
	bool                      force_refill;
	jack_nframes_t            capture_start_frame;
	jack_nframes_t            capture_captured;
	bool                      was_recording;
	jack_nframes_t            adjust_capture_position;
	jack_nframes_t           _capture_offset;
	jack_nframes_t           _roll_delay;
	jack_nframes_t            first_recordable_frame;
	jack_nframes_t            last_recordable_frame;
	int                       last_possibly_recording;
	AlignStyle               _alignment_style;
	bool                     _scrubbing;
	bool                     _slaved;
	bool                     _processed;
	Location*                 loop_location;
	jack_nframes_t            overwrite_frame;
	off_t                     overwrite_offset;
	bool                      pending_overwrite;
	bool                      overwrite_queued;
	IOChange                  input_change_pending;
	jack_nframes_t            wrap_buffer_size;
	jack_nframes_t            speed_buffer_size;

	uint64_t                  last_phase;
	uint64_t                  phi;
	
	jack_nframes_t            file_frame;		
	jack_nframes_t            playback_sample;
	jack_nframes_t            playback_distance;

	uint32_t                 _read_data_count;
	uint32_t                 _write_data_count;

	bool                      in_set_state;
	AlignStyle               _persistent_alignment_style;
	bool                      first_input_change;

	Glib::Mutex  state_lock;

	jack_nframes_t scrub_start;
	jack_nframes_t scrub_buffer_size;
	jack_nframes_t scrub_offset;

	uint32_t _refcnt;

	sigc::connection ports_created_c;
	sigc::connection plmod_connection;
	sigc::connection plstate_connection;
	sigc::connection plgone_connection;
	
	unsigned char _flags;
};

}; /* namespace ARDOUR */

#endif /* __ardour_diskstream_h__ */