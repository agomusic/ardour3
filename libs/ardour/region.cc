/*
    Copyright (C) 2000-2003 Paul Davis 

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

#include <iostream>
#include <cmath>
#include <climits>
#include <algorithm>

#include <sigc++/bind.h>
#include <sigc++/class_slot.h>

#include <glibmm/thread.h>
#include <pbd/xml++.h>

#include <ardour/region.h>
#include <ardour/playlist.h>
#include <ardour/session.h>
#include <ardour/source.h>
#include <ardour/region_factory.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

Change Region::FadeChanged       = ARDOUR::new_change ();
Change Region::SyncOffsetChanged = ARDOUR::new_change ();
Change Region::MuteChanged       = ARDOUR::new_change ();
Change Region::OpacityChanged    = ARDOUR::new_change ();
Change Region::LockChanged       = ARDOUR::new_change ();
Change Region::LayerChanged      = ARDOUR::new_change ();
Change Region::HiddenChanged     = ARDOUR::new_change ();

/** Basic Region constructor (single source) */
Region::Region (boost::shared_ptr<Source> src, jack_nframes_t start, jack_nframes_t length, const string& name, DataType type, layer_t layer, Region::Flag flags)
	: _name(name)
	, _type(type)
	, _flags(flags)
	, _start(start) 
	, _length(length) 
	, _position(0) 
	, _sync_position(_start)
	, _layer(layer)
	, _first_edit(EditChangesNothing)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change (0))
	, _last_layer_op(0)
	, _playlist(0)
{
	_current_state_id = 0;
	
	_sources.push_back (src);
	_master_sources.push_back (src);
	src->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), src));

	assert(_sources.size() > 0);
}

/** Basic Region constructor (many sources) */
Region::Region (SourceList& srcs, jack_nframes_t start, jack_nframes_t length, const string& name, DataType type, layer_t layer, Region::Flag flags)
	: _name(name)
	, _type(type)
	, _flags(flags)
	, _start(start) 
	, _length(length) 
	, _position(0) 
	, _sync_position(_start)
	, _layer(layer)
	, _first_edit(EditChangesNothing)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change (0))
	, _last_layer_op(0)
	, _playlist(0)
{
	_current_state_id = 0;
	
	set<boost::shared_ptr<Source> > unique_srcs;

	for (SourceList::iterator i=srcs.begin(); i != srcs.end(); ++i) {
		_sources.push_back (*i);
		(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		unique_srcs.insert (*i);
	}

	for (SourceList::iterator i = srcs.begin(); i != srcs.end(); ++i) {
		_master_sources.push_back (*i);
		if (unique_srcs.find (*i) == unique_srcs.end()) {
			(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		}
	}
	
	assert(_sources.size() > 0);
}

/** Create a new Region from part of an existing one */
Region::Region (boost::shared_ptr<const Region> other, jack_nframes_t offset, jack_nframes_t length, const string& name, layer_t layer, Flag flags)
	: _name(name)
	, _type(other->data_type())
	, _flags(Flag(flags & ~(Locked|WholeFile|Hidden)))
	, _start(other->_start + offset) 
	, _length(length) 
	, _position(0) 
	, _sync_position(_start)
	, _layer(layer)
	, _first_edit(EditChangesNothing)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change (0))
	, _last_layer_op(0)
	, _playlist(0)
{
	_current_state_id = 0;
	
	if (other->_sync_position < offset)
		_sync_position = other->_sync_position;

	set<boost::shared_ptr<Source> > unique_srcs;

	for (SourceList::const_iterator i= other->_sources.begin(); i != other->_sources.end(); ++i) {
		_sources.push_back (*i);
		(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		unique_srcs.insert (*i);
	}
	
	if (other->_sync_position < offset) {
		_sync_position = other->_sync_position;
	}

	for (SourceList::const_iterator i = other->_master_sources.begin(); i != other->_master_sources.end(); ++i) {
		if (unique_srcs.find (*i) == unique_srcs.end()) {
			(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		}
		_master_sources.push_back (*i);
	}
	
	assert(_sources.size() > 0);
}

/** Pure copy constructor */
Region::Region (boost::shared_ptr<const Region> other)
	: _name(other->_name)
	, _type(other->data_type())
	, _flags(Flag(other->_flags & ~Locked))
	, _start(other->_start) 
	, _length(other->_length) 
	, _position(other->_position) 
	, _sync_position(other->_sync_position)
	, _layer(other->_layer)
	, _first_edit(EditChangesID)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change(0))
	, _last_layer_op(other->_last_layer_op)
	, _playlist(0)
{
	_current_state_id = 0;
	
	other->_first_edit = EditChangesName;

	if (other->_extra_xml) {
		_extra_xml = new XMLNode (*other->_extra_xml);
	} else {
		_extra_xml = 0;
	}

	set<boost::shared_ptr<Source> > unique_srcs;

	for (SourceList::const_iterator i = other->_sources.begin(); i != other->_sources.end(); ++i) {
		_sources.push_back (*i);
		(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		unique_srcs.insert (*i);
	}

	for (SourceList::const_iterator i = other->_master_sources.begin(); i != other->_master_sources.end(); ++i) {
		_master_sources.push_back (*i);
		if (unique_srcs.find (*i) == unique_srcs.end()) {
			(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		}
	}
	
	assert(_sources.size() > 0);
}

Region::Region (SourceList& srcs, const XMLNode& node)
	: _name(X_("error: XML did not reset this"))
	, _type(DataType::NIL) // to be loaded from XML
	, _flags(Flag(0))
	, _start(0) 
	, _length(0) 
	, _position(0) 
	, _sync_position(_start)
	, _layer(0)
	, _first_edit(EditChangesNothing)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change(0))
	, _last_layer_op(0)
	, _playlist(0)

{
	_current_state_id = 0;

	set<boost::shared_ptr<Source> > unique_srcs;

	for (SourceList::iterator i=srcs.begin(); i != srcs.end(); ++i) {
		_sources.push_back (*i);
		(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		unique_srcs.insert (*i);
	}

	for (SourceList::iterator i = srcs.begin(); i != srcs.end(); ++i) {
		_master_sources.push_back (*i);
		if (unique_srcs.find (*i) == unique_srcs.end()) {
			(*i)->GoingAway.connect (bind (mem_fun (*this, &Region::source_deleted), (*i)));
		}
	}

	if (set_state (node)) {
		throw failed_constructor();
	}

	assert(_type != DataType::NIL);
	assert(_sources.size() > 0);
}

Region::Region (boost::shared_ptr<Source> src, const XMLNode& node)
	: _name(X_("error: XML did not reset this"))
	, _type(DataType::NIL)
	, _flags(Flag(0))
	, _start(0) 
	, _length(0) 
	, _position(0) 
	, _sync_position(_start)
	, _layer(0)
	, _first_edit(EditChangesNothing)
	, _frozen(0)
	, _read_data_count(0)
	, _pending_changed(Change(0))
	, _last_layer_op(0)
	, _playlist(0)
{
	_sources.push_back (src);

	_current_state_id = 0;

	if (set_state (node)) {
		throw failed_constructor();
	}
	
	assert(_type != DataType::NIL);
	assert(_sources.size() > 0);
}

Region::~Region ()
{
	notify_callbacks ();

	/* derived classes must emit GoingAway */
}

void
Region::set_playlist (Playlist* pl)
{
	_playlist = pl;
}

void
Region::store_state (RegionState& state) const
{
	state._start = _start;
	state._length = _length;
	state._position = _position;
	state._flags = _flags;
	state._sync_position = _sync_position;
	state._layer = _layer;
	state._name = _name;
	state._first_edit = _first_edit;
}	

Change
Region::restore_and_return_flags (RegionState& state)
{
	Change what_changed = Change (0);

	{
		Glib::Mutex::Lock lm (_lock);
		
		if (_start != state._start) {
			what_changed = Change (what_changed|StartChanged);	
			_start = state._start;
		}
		if (_length != state._length) {
			what_changed = Change (what_changed|LengthChanged);
			_length = state._length;
		}
		if (_position != state._position) {
			what_changed = Change (what_changed|PositionChanged);
			_position = state._position;
		} 
		if (_sync_position != state._sync_position) {
			_sync_position = state._sync_position;
			what_changed = Change (what_changed|SyncOffsetChanged);
		}
		if (_layer != state._layer) {
			what_changed = Change (what_changed|LayerChanged);
			_layer = state._layer;
		}

		uint32_t old_flags = _flags;
		_flags = Flag (state._flags);
		
		if ((old_flags ^ state._flags) & Muted) {
			what_changed = Change (what_changed|MuteChanged);
		}
		if ((old_flags ^ state._flags) & Opaque) {
			what_changed = Change (what_changed|OpacityChanged);
		}
		if ((old_flags ^ state._flags) & Locked) {
			what_changed = Change (what_changed|LockChanged);
		}

		_first_edit = state._first_edit;
	}

	return what_changed;
}

void
Region::set_name (string str)

{
	if (_name != str) {
		_name = str; 
		send_change (NameChanged);
	}
}

void
Region::set_length (jack_nframes_t len, void *src)
{
	if (_flags & Locked) {
		return;
	}

	if (_length != len && len != 0) {

		if (!verify_length (len)) {
			return;
		}
		
		_length = len;

		_flags = Region::Flag (_flags & ~WholeFile);

		first_edit ();
		maybe_uncopy ();

		if (!_frozen) {
			recompute_at_end ();

			char buf[64];
			snprintf (buf, sizeof (buf), "length set to %u", len);
			save_state (buf);
		}

		send_change (LengthChanged);
	}
}

void
Region::maybe_uncopy ()
{
}

void
Region::first_edit ()
{
	if (_first_edit != EditChangesNothing && _playlist) {

		_name = _playlist->session().new_region_name (_name);
		_first_edit = EditChangesNothing;

		send_change (NameChanged);
		RegionFactory::CheckNewRegion (shared_from_this());
	}
}

void
Region::move_to_natural_position (void *src)
{
	if (!_playlist) {
		return;
	}

	boost::shared_ptr<Region> whole_file_region = get_parent();

	if (whole_file_region) {
		set_position (whole_file_region->position() + _start, src);
	}
}
	
void
Region::special_set_position (jack_nframes_t pos)
{
	/* this is used when creating a whole file region as 
	   a way to store its "natural" or "captured" position.
	*/

	_position = pos;
}

void
Region::set_position (jack_nframes_t pos, void *src)
{
	if (_flags & Locked) {
		return;
	}

	if (_position != pos) {
		_position = pos;

		if (!_frozen) {
			char buf[64];
			snprintf (buf, sizeof (buf), "position set to %u", pos);
			save_state (buf);
		}
	}

	/* do this even if the position is the same. this helps out
	   a GUI that has moved its representation already.
	*/

	send_change (PositionChanged);
}

void
Region::set_position_on_top (jack_nframes_t pos, void *src)
{
	if (_flags & Locked) {
		return;
	}

	if (_position != pos) {
		_position = pos;

		if (!_frozen) {
			char buf[64];
			snprintf (buf, sizeof (buf), "position set to %u", pos);
			save_state (buf);
		}
	}

	_playlist->raise_region_to_top (boost::shared_ptr<Region>(this));

	/* do this even if the position is the same. this helps out
	   a GUI that has moved its representation already.
	*/
	
	send_change (PositionChanged);
}

void
Region::nudge_position (long n, void *src)
{
	if (_flags & Locked) {
		return;
	}

	if (n == 0) {
		return;
	}
	
	if (n > 0) {
		if (_position > max_frames - n) {
			_position = max_frames;
		} else {
			_position += n;
		}
	} else {
		if (_position < (jack_nframes_t) -n) {
			_position = 0;
		} else {
			_position += n;
		}
	}

	if (!_frozen) {
		char buf[64];
		snprintf (buf, sizeof (buf), "position set to %u", _position);
		save_state (buf);
	}

	send_change (PositionChanged);
}

void
Region::set_start (jack_nframes_t pos, void *src)
{
	if (_flags & Locked) {
		return;
	}
	/* This just sets the start, nothing else. It effectively shifts
	   the contents of the Region within the overall extent of the Source,
	   without changing the Region's position or length
	*/

	if (_start != pos) {

		if (!verify_start (pos)) {
			return;
		}

		_start = pos;
		_flags = Region::Flag (_flags & ~WholeFile);
		first_edit ();

		if (!_frozen) {
			char buf[64];
			snprintf (buf, sizeof (buf), "start set to %u", pos);
			save_state (buf);
		}

		send_change (StartChanged);
	}
}

void
Region::trim_start (jack_nframes_t new_position, void *src)
{
	if (_flags & Locked) {
		return;
	}
	jack_nframes_t new_start;
	int32_t start_shift;
	
	if (new_position > _position) {
		start_shift = new_position - _position;
	} else {
		start_shift = -(_position - new_position);
	}

	if (start_shift > 0) {

		if (_start > max_frames - start_shift) {
			new_start = max_frames;
		} else {
			new_start = _start + start_shift;
		}

		if (!verify_start (new_start)) {
			return;
		}

	} else if (start_shift < 0) {

		if (_start < (jack_nframes_t) -start_shift) {
			new_start = 0;
		} else {
			new_start = _start + start_shift;
		}
	} else {
		return;
	}

	if (new_start == _start) {
		return;
	}
	
	_start = new_start;
	_flags = Region::Flag (_flags & ~WholeFile);
	first_edit ();

	if (!_frozen) {
		char buf[64];
		snprintf (buf, sizeof (buf), "slipped start to %u", _start);
		save_state (buf);
	}

	send_change (StartChanged);
}

void
Region::trim_front (jack_nframes_t new_position, void *src)
{
	if (_flags & Locked) {
		return;
	}

	jack_nframes_t end = _position + _length - 1;
	jack_nframes_t source_zero;

	if (_position > _start) {
		source_zero = _position - _start;
	} else {
		source_zero = 0; // its actually negative, but this will work for us
	}

	if (new_position < end) { /* can't trim it zero or negative length */
		
		jack_nframes_t newlen;

		/* can't trim it back passed where source position zero is located */
		
		new_position = max (new_position, source_zero);
		
		
		if (new_position > _position) {
			newlen = _length - (new_position - _position);
		} else {
			newlen = _length + (_position - new_position);
		}
		
		trim_to_internal (new_position, newlen, src);
		if (!_frozen) {
			recompute_at_start ();
		}
	}
}

void
Region::trim_end (jack_nframes_t new_endpoint, void *src)
{
	if (_flags & Locked) {
		return;
	}

	if (new_endpoint > _position) {
		trim_to_internal (_position, new_endpoint - _position, this);
		if (!_frozen) {
			recompute_at_end ();
		}
	}
}

void
Region::trim_to (jack_nframes_t position, jack_nframes_t length, void *src)
{
	if (_flags & Locked) {
		return;
	}

	trim_to_internal (position, length, src);

	if (!_frozen) {
		recompute_at_start ();
		recompute_at_end ();
	}
}

void
Region::trim_to_internal (jack_nframes_t position, jack_nframes_t length, void *src)
{
	int32_t start_shift;
	jack_nframes_t new_start;

	if (_flags & Locked) {
		return;
	}

	if (position > _position) {
		start_shift = position - _position;
	} else {
		start_shift = -(_position - position);
	}

	if (start_shift > 0) {

		if (_start > max_frames - start_shift) {
			new_start = max_frames;
		} else {
			new_start = _start + start_shift;
		}


	} else if (start_shift < 0) {

		if (_start < (jack_nframes_t) -start_shift) {
			new_start = 0;
		} else {
			new_start = _start + start_shift;
		}
	} else {
		new_start = _start;
	}

	if (!verify_start_and_length (new_start, length)) {
		return;
	}

	Change what_changed = Change (0);

	if (_start != new_start) {
		_start = new_start;
		what_changed = Change (what_changed|StartChanged);
	}
	if (_length != length) {
		_length = length;
		what_changed = Change (what_changed|LengthChanged);
	}
	if (_position != position) {
		_position = position;
		what_changed = Change (what_changed|PositionChanged);
	}
	
	_flags = Region::Flag (_flags & ~WholeFile);

	if (what_changed & (StartChanged|LengthChanged)) {
		first_edit ();
	} 

	if (what_changed) {
		
		if (!_frozen) {
			char buf[64];
			snprintf (buf, sizeof (buf), "trimmed to %u-%u", _position, _position+_length-1);
			save_state (buf);
		}

		send_change (what_changed);
	}
}	

void
Region::set_hidden (bool yn)
{
	if (hidden() != yn) {

		if (yn) {
			_flags = Flag (_flags|Hidden);
		} else {
			_flags = Flag (_flags & ~Hidden);
		}

		send_change (HiddenChanged);
	}
}

void
Region::set_muted (bool yn)
{
	if (muted() != yn) {

		if (yn) {
			_flags = Flag (_flags|Muted);
		} else {
			_flags = Flag (_flags & ~Muted);
		}

		if (!_frozen) {
			char buf[64];
			if (yn) {
				snprintf (buf, sizeof (buf), "muted");
			} else {
				snprintf (buf, sizeof (buf), "unmuted");
			}
			save_state (buf);
		}

		send_change (MuteChanged);
	}
}

void
Region::set_opaque (bool yn)
{
	if (opaque() != yn) {
		if (!_frozen) {
			char buf[64];
			if (yn) {
				snprintf (buf, sizeof (buf), "opaque");
				_flags = Flag (_flags|Opaque);
			} else {
				snprintf (buf, sizeof (buf), "translucent");
				_flags = Flag (_flags & ~Opaque);
			}
			save_state (buf);
		}
		send_change (OpacityChanged);
	}
}

void
Region::set_locked (bool yn)
{
	if (locked() != yn) {
		if (!_frozen) {
			char buf[64];
			if (yn) {
				snprintf (buf, sizeof (buf), "locked");
				_flags = Flag (_flags|Locked);
			} else {
				snprintf (buf, sizeof (buf), "unlocked");
				_flags = Flag (_flags & ~Locked);
			}
			save_state (buf);
		}
		send_change (LockChanged);
	}
}

void
Region::set_sync_position (jack_nframes_t absolute_pos)
{
	jack_nframes_t file_pos;

	file_pos = _start + (absolute_pos - _position);

	if (file_pos != _sync_position) {
		
		_sync_position = file_pos;
		_flags = Flag (_flags|SyncMarked);

		if (!_frozen) {
			char buf[64];
			maybe_uncopy ();
			snprintf (buf, sizeof (buf), "sync point set to %u", _sync_position);
			save_state (buf);
		}
		send_change (SyncOffsetChanged);
	}
}

void
Region::clear_sync_position ()
{
	if (_flags & SyncMarked) {
		_flags = Flag (_flags & ~SyncMarked);

		if (!_frozen) {
			maybe_uncopy ();
			save_state ("sync point removed");
		}
		send_change (SyncOffsetChanged);
	}
}

jack_nframes_t
Region::sync_offset (int& dir) const
{
	/* returns the sync point relative the first frame of the region */

	if (_flags & SyncMarked) {
		if (_sync_position > _start) {
			dir = 1;
			return _sync_position - _start; 
		} else {
			dir = -1;
			return _start - _sync_position;
		}
	} else {
		dir = 0;
		return 0;
	}
}

jack_nframes_t 
Region::adjust_to_sync (jack_nframes_t pos)
{
	int sync_dir;
	jack_nframes_t offset = sync_offset (sync_dir);
	
	if (sync_dir > 0) {
		if (max_frames - pos > offset) {
			pos += offset;
		}
	} else {
		if (pos > offset) {
			pos -= offset;
		} else {
			pos = 0;
		}
	}

	return pos;
}

jack_nframes_t
Region::sync_position() const
{
	if (_flags & SyncMarked) {
		return _sync_position; 
	} else {
		return _start;
	}
}


void
Region::raise ()
{
	if (_playlist == 0) {
		return;
	}

	_playlist->raise_region (boost::shared_ptr<Region>(this));
}

void
Region::lower ()
{
	if (_playlist == 0) {
		return;
	}

	_playlist->lower_region (boost::shared_ptr<Region>(this));
}

void
Region::raise_to_top ()
{

	if (_playlist == 0) {
		return;
	}

	_playlist->raise_region_to_top (boost::shared_ptr<Region>(this));
}

void
Region::lower_to_bottom ()
{
	if (_playlist == 0) {
		return;
	}

	_playlist->lower_region_to_bottom (boost::shared_ptr<Region>(this));
}

void
Region::set_layer (layer_t l)
{
	if (_layer != l) {
		_layer = l;
		
		if (!_frozen) {
			char buf[64];
			snprintf (buf, sizeof (buf), "layer set to %" PRIu32, _layer);
			save_state (buf);
		}
		
		send_change (LayerChanged);
	}
}

XMLNode&
Region::state (bool full_state)
{
	XMLNode *node = new XMLNode ("Region");
	char buf[64];
	
	_id.print (buf);
	node->add_property ("id", buf);
	node->add_property ("name", _name);
	node->add_property ("type", _type.to_string());
	snprintf (buf, sizeof (buf), "%u", _start);
	node->add_property ("start", buf);
	snprintf (buf, sizeof (buf), "%u", _length);
	node->add_property ("length", buf);
	snprintf (buf, sizeof (buf), "%u", _position);
	node->add_property ("position", buf);

	/* note: flags are stored by derived classes */

	snprintf (buf, sizeof (buf), "%d", (int) _layer);
	node->add_property ("layer", buf);
	snprintf (buf, sizeof (buf), "%u", _sync_position);
	node->add_property ("sync-position", buf);

	return *node;
}

XMLNode&
Region::get_state ()
{
	return state (true);
}

int
Region::set_state (const XMLNode& node)
{
	const XMLNodeList& nlist = node.children();
	const XMLProperty *prop;

	if (_extra_xml) {
		delete _extra_xml;
		_extra_xml = 0;
	}

	if ((prop = node.property ("id")) == 0) {
		error << _("Session: XMLNode describing a Region is incomplete (no id)") << endmsg;
		return -1;
	}

	_id = prop->value();

	if ((prop = node.property ("name")) == 0) {
		error << _("Session: XMLNode describing a Region is incomplete (no name)") << endmsg;
		return -1;
	}

	_name = prop->value();
	
	if ((prop = node.property ("type")) == 0) {
		_type = DataType::AUDIO;
	} else {
		_type = DataType(prop->value());
	}

	if ((prop = node.property ("start")) != 0) {
		_start = (jack_nframes_t) atoi (prop->value().c_str());
	}

	if ((prop = node.property ("length")) != 0) {
		_length = (jack_nframes_t) atoi (prop->value().c_str());
	}

	if ((prop = node.property ("position")) != 0) {
		_position = (jack_nframes_t) atoi (prop->value().c_str());
	}

	if ((prop = node.property ("layer")) != 0) {
		_layer = (layer_t) atoi (prop->value().c_str());
	}

	/* note: derived classes set flags */

	if ((prop = node.property ("sync-position")) != 0) {
		_sync_position = (jack_nframes_t) atoi (prop->value().c_str());
	} else {
		_sync_position = _start;
	}
	
	for (XMLNodeConstIterator niter = nlist.begin(); niter != nlist.end(); ++niter) {
		
		XMLNode *child;
		
		child = (*niter);
		
		if (child->name () == "extra") {
			_extra_xml = new XMLNode (*child);
			break;
		}
	}

	_first_edit = EditChangesNothing;

	return 0;
}

void
Region::freeze ()
{
	_frozen++;
}

void
Region::thaw (const string& why)
{
	Change what_changed = Change (0);

	{
		Glib::Mutex::Lock lm (_lock);

		if (_frozen && --_frozen > 0) {
			return;
		}

		if (_pending_changed) {
			what_changed = _pending_changed;
			_pending_changed = Change (0);
		}
	}

	if (what_changed == Change (0)) {
		return;
	}

	if (what_changed & LengthChanged) {
		if (what_changed & PositionChanged) {
			recompute_at_start ();
		} 
		recompute_at_end ();
	}
		
	save_state (why);
	StateChanged (what_changed);
}

void
Region::send_change (Change what_changed)
{
	{
		Glib::Mutex::Lock lm (_lock);
		if (_frozen) {
			_pending_changed = Change (_pending_changed|what_changed);
			return;
		} 
	}

	StateManager::send_state_changed (what_changed);
}

void
Region::set_last_layer_op (uint64_t when)
{
	_last_layer_op = when;
}

bool
Region::overlap_equivalent (boost::shared_ptr<const Region> other) const
{
	return coverage (other->first_frame(), other->last_frame()) != OverlapNone;
}

bool
Region::equivalent (boost::shared_ptr<const Region> other) const
{
	return _start == other->_start &&
		_position == other->_position &&
		_length == other->_length;
}

bool
Region::size_equivalent (boost::shared_ptr<const Region> other) const
{
	return _start == other->_start &&
		_length == other->_length;
}

bool
Region::region_list_equivalent (boost::shared_ptr<const Region> other) const
{
	return size_equivalent (other) && source_equivalent (other) && _name == other->_name;
}

void
Region::source_deleted (boost::shared_ptr<Source>)
{
	delete this;
}

vector<string>
Region::master_source_names ()
{
	SourceList::iterator i;

	vector<string> names;
	for (i = _master_sources.begin(); i != _master_sources.end(); ++i) {
		names.push_back((*i)->name());
	}

	return names;
}

bool
Region::source_equivalent (boost::shared_ptr<const Region> other) const
{
	if (!other)
		return false;

	SourceList::const_iterator i;
	SourceList::const_iterator io;

	for (i = _sources.begin(), io = other->_sources.begin(); i != _sources.end() && io != other->_sources.end(); ++i, ++io) {
		if ((*i)->id() != (*io)->id()) {
			return false;
		}
	}

	for (i = _master_sources.begin(), io = other->_master_sources.begin(); i != _master_sources.end() && io != other->_master_sources.end(); ++i, ++io) {
		if ((*i)->id() != (*io)->id()) {
			return false;
		}
	}

	return true;
}

bool
Region::verify_length (jack_nframes_t len)
{
	for (uint32_t n=0; n < _sources.size(); ++n) {
		if (_start > _sources[n]->length() - len) {
			return false;
		}
	}
	return true;
}

bool
Region::verify_start_and_length (jack_nframes_t new_start, jack_nframes_t new_length)
{
	for (uint32_t n=0; n < _sources.size(); ++n) {
		if (new_length > _sources[n]->length() - new_start) {
			return false;
		}
	}
	return true;
}
bool
Region::verify_start (jack_nframes_t pos)
{
	for (uint32_t n=0; n < _sources.size(); ++n) {
		if (pos > _sources[n]->length() - _length) {
			return false;
		}
	}
	return true;
}

bool
Region::verify_start_mutable (jack_nframes_t& new_start)
{
	for (uint32_t n=0; n < _sources.size(); ++n) {
		if (new_start > _sources[n]->length() - _length) {
			new_start = _sources[n]->length() - _length;
		}
	}
	return true;
}

boost::shared_ptr<Region>
Region::get_parent()
{
	boost::shared_ptr<Region> r;

	if (_playlist) {
		r = _playlist->session().find_whole_file_parent (*this);
	}
	
	return r;
}

