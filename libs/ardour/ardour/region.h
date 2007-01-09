/*
    Copyright (C) 2000-2001 Paul Davis 

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

#ifndef __ardour_region_h__
#define __ardour_region_h__

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <pbd/undo.h>
#include <pbd/statefuldestructible.h> 

#include <ardour/ardour.h>
#include <ardour/data_type.h>

class XMLNode;

namespace ARDOUR {

class Playlist;

enum RegionEditState {
	EditChangesNothing = 0,
	EditChangesName    = 1,
	EditChangesID      = 2
};

class Region : public PBD::StatefulDestructible, public boost::enable_shared_from_this<Region>
{
  public:
	typedef std::vector<boost::shared_ptr<Source> > SourceList;

	enum Flag {
		Muted = 0x1,
		Opaque = 0x2,
		EnvelopeActive = 0x4,
		DefaultFadeIn = 0x8,
		DefaultFadeOut = 0x10,
		Locked = 0x20,
		Automatic = 0x40,
		WholeFile = 0x80,
		FadeIn = 0x100,
		FadeOut = 0x200,
		Copied = 0x400,
		Import = 0x800,
		External = 0x1000,
		SyncMarked = 0x2000,
		LeftOfSplit = 0x4000,
		RightOfSplit = 0x8000,
		Hidden = 0x10000,
		DoNotSaveState = 0x20000,
		//
		range_guarantoor = USHRT_MAX
	};

 	static const Flag DefaultFlags = Flag (Opaque|DefaultFadeIn|DefaultFadeOut|FadeIn|FadeOut);

	static Change FadeChanged;
	static Change SyncOffsetChanged;
	static Change MuteChanged;
	static Change OpacityChanged;
	static Change LockChanged;
	static Change LayerChanged;
	static Change HiddenChanged;

	sigc::signal<void,Change> StateChanged;

	virtual ~Region();

	/* Note: changing the name of a Region does not constitute an edit */

	string name() const { return _name; }
	void set_name (string str);

	const DataType& data_type() const { return _type; }

	nframes_t position () const { return _position; }
	nframes_t start ()    const { return _start; }
	nframes_t length()    const { return _length; }
	layer_t   layer ()    const { return _layer; }
	
	nframes_t sync_offset(int& dir) const;
	nframes_t sync_position() const;

	nframes_t adjust_to_sync (nframes_t);
	
	/* first_frame() is an alias; last_frame() just hides some math */

	nframes_t first_frame() const { return _position; }
	nframes_t last_frame() const { return _position + _length - 1; }

	Flag flags()      const { return _flags; }
	bool hidden()     const { return _flags & Hidden; }
	bool muted()      const { return _flags & Muted; }
	bool opaque ()    const { return _flags & Opaque; }
	bool locked()     const { return _flags & Locked; }
	bool automatic()  const { return _flags & Automatic; }
	bool whole_file() const { return _flags & WholeFile ; }
	bool captured()   const { return !(_flags & (Region::Flag (Region::Import|Region::External))); }

	virtual bool should_save_state () const { return !(_flags & DoNotSaveState); };

	void freeze ();
	void thaw (const string& why);

	bool covers (nframes_t frame) const {
		return first_frame() <= frame && frame < last_frame();
	}

	OverlapType coverage (nframes_t start, nframes_t end) const {
		return ARDOUR::coverage (first_frame(), last_frame(), start, end);
	}
	
	bool equivalent (boost::shared_ptr<const Region>) const;
	bool size_equivalent (boost::shared_ptr<const Region>) const;
	bool overlap_equivalent (boost::shared_ptr<const Region>) const;
	bool region_list_equivalent (boost::shared_ptr<const Region>) const;
	bool source_equivalent (boost::shared_ptr<const Region>) const;
	
	/* EDITING OPERATIONS */

	void set_length (nframes_t, void *src);
	void set_start (nframes_t, void *src);
	void set_position (nframes_t, void *src);
	void set_position_on_top (nframes_t, void *src);
	void special_set_position (nframes_t);
	void nudge_position (long, void *src);

	bool at_natural_position () const;
	void move_to_natural_position (void *src);

	void trim_start (nframes_t new_position, void *src);
	void trim_front (nframes_t new_position, void *src);
	void trim_end (nframes_t new_position, void *src);
	void trim_to (nframes_t position, nframes_t length, void *src);
	
	void set_layer (layer_t l); /* ONLY Playlist can call this */
	void raise ();
	void lower ();
	void raise_to_top ();
	void lower_to_bottom ();

	void set_sync_position (nframes_t n);
	void clear_sync_position ();
	void set_hidden (bool yn);
	void set_muted (bool yn);
	void set_opaque (bool yn);
	void set_locked (bool yn);

	virtual uint32_t read_data_count() const { return _read_data_count; }

	boost::shared_ptr<ARDOUR::Playlist> playlist() const { return _playlist.lock(); }
	virtual void set_playlist (boost::weak_ptr<ARDOUR::Playlist>);

	void source_deleted (boost::shared_ptr<Source>);
	
	boost::shared_ptr<Source> source (uint32_t n=0) const { return _sources[ (n < _sources.size()) ? n : 0 ]; }
	uint32_t                  n_channels()          const { return _sources.size(); }

	std::vector<string> master_source_names();


	/* serialization */
	
	XMLNode&         get_state ();
	virtual XMLNode& state (bool);
	virtual int      set_state (const XMLNode&);
	virtual int      set_live_state (const XMLNode&, Change&, bool send);

	virtual boost::shared_ptr<Region> get_parent() const;
	
	uint64_t last_layer_op() const { return _last_layer_op; }
	void set_last_layer_op (uint64_t when);

  protected:
	friend class RegionFactory;

	Region (boost::shared_ptr<Source> src, nframes_t start, nframes_t length, 
		const string& name, DataType type, layer_t = 0, Flag flags = DefaultFlags);
	Region (SourceList& srcs, nframes_t start, nframes_t length, 
		const string& name, DataType type, layer_t = 0, Flag flags = DefaultFlags);
	
	Region (boost::shared_ptr<const Region>, nframes_t start, nframes_t length, const string& name, layer_t = 0, Flag flags = DefaultFlags);
	Region (boost::shared_ptr<const Region>);
	Region (boost::shared_ptr<Source> src, const XMLNode&);
	Region (SourceList& srcs, const XMLNode&);

  protected:
	XMLNode& get_short_state (); /* used only by Session */

	void send_change (Change);

	void trim_to_internal (nframes_t position, nframes_t length, void *src);

	bool copied() const { return _flags & Copied; }
	void maybe_uncopy ();
	void first_edit ();
	
	virtual bool verify_start (jack_nframes_t);
	virtual bool verify_start_and_length (jack_nframes_t, jack_nframes_t);
	virtual bool verify_start_mutable (jack_nframes_t&_start);
	virtual bool verify_length (jack_nframes_t);
	virtual void recompute_at_start () = 0;
	virtual void recompute_at_end () = 0;
	

	PBD::ID                 _id;
	string                  _name;
	DataType                _type;
	Flag                    _flags;
	nframes_t          _start;
	nframes_t          _length;
	nframes_t          _position;
	nframes_t          _sync_position;
	layer_t                 _layer;
	mutable RegionEditState _first_edit;
	int                     _frozen;
	mutable uint32_t        _read_data_count;  ///< modified in read()
	Change                  _pending_changed;
	uint64_t                _last_layer_op;  ///< timestamp
	Glib::Mutex             _lock;
	boost::weak_ptr<ARDOUR::Playlist> _playlist;
	SourceList              _sources;
	/** Used when timefx are applied, so we can always use the original source */
	SourceList              _master_sources;
};

} /* namespace ARDOUR */

#endif /* __ardour_region_h__ */
