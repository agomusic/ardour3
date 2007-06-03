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

#include <cassert>

#include <algorithm>

#include <stdlib.h>

#include <sigc++/bind.h>

#include <ardour/types.h>
#include <ardour/configuration.h>
#include <ardour/midi_playlist.h>
#include <ardour/midi_region.h>
#include <ardour/session.h>
#include <ardour/midi_ring_buffer.h>

#include <pbd/error.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace sigc;
using namespace std;

MidiPlaylist::MidiPlaylist (Session& session, const XMLNode& node, bool hidden)
		: Playlist (session, node, DataType::MIDI, hidden)
{
	const XMLProperty* prop = node.property("type");
	assert(prop && DataType(prop->value()) == DataType::MIDI);

	in_set_state++;
	set_state (node);
	in_set_state--;
}

MidiPlaylist::MidiPlaylist (Session& session, string name, bool hidden)
		: Playlist (session, name, DataType::MIDI, hidden)
{
}

MidiPlaylist::MidiPlaylist (boost::shared_ptr<const MidiPlaylist> other, string name, bool hidden)
		: Playlist (other, name, hidden)
{
	throw; // nope

	/*
	list<Region*>::const_iterator in_o  = other.regions.begin();
	list<Region*>::iterator in_n = regions.begin();

	while (in_o != other.regions.end()) {
		MidiRegion *ar = dynamic_cast<MidiRegion *>( (*in_o) );

		for (list<Crossfade *>::const_iterator xfades = other._crossfades.begin(); xfades != other._crossfades.end(); ++xfades) {
			if ( &(*xfades)->in() == ar) {
				// We found one! Now copy it!

				list<Region*>::const_iterator out_o = other.regions.begin();
				list<Region*>::const_iterator out_n = regions.begin();

				while (out_o != other.regions.end()) {

					MidiRegion *ar2 = dynamic_cast<MidiRegion *>( (*out_o) );

					if ( &(*xfades)->out() == ar2) {
						MidiRegion *in  = dynamic_cast<MidiRegion*>( (*in_n) );
						MidiRegion *out = dynamic_cast<MidiRegion*>( (*out_n) );
						Crossfade *new_fade = new Crossfade( *(*xfades), in, out);
						add_crossfade(*new_fade);
						break;
					}

					out_o++;
					out_n++;
				}
				//				cerr << "HUH!? second region in the crossfade not found!" << endl;
			}
		}

		in_o++;
		in_n++;
	}
*/
}

MidiPlaylist::MidiPlaylist (boost::shared_ptr<const MidiPlaylist> other, nframes_t start, nframes_t dur, string name, bool hidden)
		: Playlist (other, start, dur, name, hidden)
{
	/* this constructor does NOT notify others (session) */
}

MidiPlaylist::~MidiPlaylist ()
{
  	GoingAway (); /* EMIT SIGNAL */
	
	/* drop connections to signals */
	
	notify_callbacks ();
}

struct RegionSortByLayer {
    bool operator() (boost::shared_ptr<Region> a, boost::shared_ptr<Region> b) {
	    return a->layer() < b->layer();
    }
};

/** Returns the number of frames in time duration read (eg could be large when 0 events are read) */
nframes_t
MidiPlaylist::read (MidiRingBuffer& dst, nframes_t start,
                     nframes_t dur, unsigned chan_n)
{
	/* this function is never called from a realtime thread, so
	   its OK to block (for short intervals).
	*/

	Glib::Mutex::Lock rm (region_lock);

	nframes_t end = start + dur - 1;

	_read_data_count = 0;

	// relevent regions overlapping start <--> end
	vector<boost::shared_ptr<Region> > regs;

	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {

		if ((*i)->coverage (start, end) != OverlapNone) {
			regs.push_back(*i);
		}
	}

	RegionSortByLayer layer_cmp;
	sort(regs.begin(), regs.end(), layer_cmp);

	for (vector<boost::shared_ptr<Region> >::iterator i = regs.begin(); i != regs.end(); ++i) {
		// FIXME: ensure time is monotonic here
		boost::shared_ptr<MidiRegion> mr = boost::dynamic_pointer_cast<MidiRegion>(*i);
		mr->read_at (dst, start, dur, chan_n);
		_read_data_count += mr->read_data_count();
	}

	return dur;
}


void
MidiPlaylist::remove_dependents (boost::shared_ptr<Region> region)
{
	/* MIDI regions have no dependents (crossfades) */
}


void
MidiPlaylist::refresh_dependents (boost::shared_ptr<Region> r)
{
	/* MIDI regions have no dependents (crossfades) */
}

void
MidiPlaylist::finalize_split_region (boost::shared_ptr<Region> original, boost::shared_ptr<Region> left, boost::shared_ptr<Region> right)
{
	throw; // I don't wanna
	/*
	MidiRegion *orig  = dynamic_cast<MidiRegion*>(o);
	MidiRegion *left  = dynamic_cast<MidiRegion*>(l);
	MidiRegion *right = dynamic_cast<MidiRegion*>(r);

	for (Crossfades::iterator x = _crossfades.begin(); x != _crossfades.end();) {
		Crossfades::iterator tmp;
		tmp = x;
		++tmp;

		Crossfade *fade = 0;

		if ((*x)->_in == orig) {
			if (! (*x)->covers(right->position())) {
				fade = new Crossfade( *(*x), left, (*x)->_out);
			} else {
				// Overlap, the crossfade is copied on the left side of the right region instead
				fade = new Crossfade( *(*x), right, (*x)->_out);
			}
		}

		if ((*x)->_out == orig) {
			if (! (*x)->covers(right->position())) {
				fade = new Crossfade( *(*x), (*x)->_in, right);
			} else {
				// Overlap, the crossfade is copied on the right side of the left region instead
				fade = new Crossfade( *(*x), (*x)->_in, left);
			}
		}

		if (fade) {
			_crossfades.remove( (*x) );
			add_crossfade (*fade);
		}
		x = tmp;
	}*/
}

void
MidiPlaylist::check_dependents (boost::shared_ptr<Region> r, bool norefresh)
{
	/* MIDI regions have no dependents (crossfades) */
}


int
MidiPlaylist::set_state (const XMLNode& node)
{
	in_set_state++;
	freeze ();

	Playlist::set_state (node);

	thaw();
	in_set_state--;

	return 0;
}

void
MidiPlaylist::dump () const
{
	boost::shared_ptr<Region> r;

	cerr << "Playlist \"" << _name << "\" " << endl
	<< regions.size() << " regions "
	<< endl;

	for (RegionList::const_iterator i = regions.begin(); i != regions.end(); ++i) {
		r = *i;
		cerr << "  " << r->name() << " @ " << r << " ["
		<< r->start() << "+" << r->length()
		<< "] at "
		<< r->position()
		<< " on layer "
		<< r->layer ()
		<< endl;
	}
}

bool
MidiPlaylist::destroy_region (boost::shared_ptr<Region> region)
{
	boost::shared_ptr<MidiRegion> r = boost::dynamic_pointer_cast<MidiRegion> (region);
	bool changed = false;

	if (r == 0) {
		PBD::fatal << _("programming error: non-midi Region passed to remove_overlap in midi playlist")
		<< endmsg;
		/*NOTREACHED*/
		return false;
	}

	{
		RegionLock rlock (this);
		RegionList::iterator i;
		RegionList::iterator tmp;

		for (i = regions.begin(); i != regions.end(); ) {

			tmp = i;
			++tmp;

			if ((*i) == region) {
				regions.erase (i);
				changed = true;
			}

			i = tmp;
		}
	}


	if (changed) {
		/* overload this, it normally means "removed", not destroyed */
		notify_region_removed (region);
	}

	return changed;
}

bool
MidiPlaylist::region_changed (Change what_changed, boost::shared_ptr<Region> region)
{
	if (in_flush || in_set_state) {
		return false;
	}

	// Feeling rather uninterested today, but thanks for the heads up anyway!
	
	Change our_interests = Change (/*MidiRegion::FadeInChanged|
	                               MidiRegion::FadeOutChanged|
	                               MidiRegion::FadeInActiveChanged|
	                               MidiRegion::FadeOutActiveChanged|
	                               MidiRegion::EnvelopeActiveChanged|
	                               MidiRegion::ScaleAmplitudeChanged|
	                               MidiRegion::EnvelopeChanged*/);
	bool parent_wants_notify;

	parent_wants_notify = Playlist::region_changed (what_changed, region);

	if ((parent_wants_notify || (what_changed & our_interests))) {
		notify_modified ();
	}

	return true;
}

