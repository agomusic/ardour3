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

#include <set>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <climits>

#include <sigc++/bind.h>

#include <pbd/failed_constructor.h>
#include <pbd/stl_delete.h>
#include <pbd/xml++.h>

#include <ardour/playlist.h>
#include <ardour/session.h>
#include <ardour/region.h>
#include <ardour/region_factory.h>
#include <ardour/playlist_factory.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

struct ShowMeTheList {
    ShowMeTheList (boost::shared_ptr<Playlist> pl, const string& n) : playlist (pl), name (n) {}
    ~ShowMeTheList () { 
	    cerr << ">>>>" << name << endl; playlist->dump(); cerr << "<<<<" << name << endl << endl; 
    };
    boost::shared_ptr<Playlist> playlist;
    string name;
};

struct RegionSortByLayer {
    bool operator() (boost::shared_ptr<Region> a, boost::shared_ptr<Region> b) {
	    return a->layer() < b->layer();
    }
};

struct RegionSortByPosition {
    bool operator() (boost::shared_ptr<Region> a, boost::shared_ptr<Region> b) {
	    return a->position() < b->position();
    }
};

struct RegionSortByLastLayerOp {
    bool operator() (boost::shared_ptr<Region> a, boost::shared_ptr<Region> b) {
	    return a->last_layer_op() < b->last_layer_op();
    }
};

Playlist::Playlist (Session& sess, string nom, bool hide)
	: _session (sess)
{
	init (hide);
	_name = nom;
	
}

Playlist::Playlist (Session& sess, const XMLNode& node, bool hide)
	: _session (sess)
{
	init (hide);
	_name = "unnamed"; /* reset by set_state */

	/* set state called by derived class */
}

Playlist::Playlist (boost::shared_ptr<const Playlist> other, string namestr, bool hide)
	: _name (namestr), _session (other->_session), _orig_diskstream_id(other->_orig_diskstream_id)
{
	init (hide);

	RegionList tmp;
	other->copy_regions (tmp);
	
	in_set_state++;

	for (list<boost::shared_ptr<Region> >::iterator x = tmp.begin(); x != tmp.end(); ++x) {
		add_region_internal( (*x), (*x)->position());
	}

	in_set_state--;

	_splicing  = other->_splicing;
	_nudging   = other->_nudging;
	_edit_mode = other->_edit_mode;

	in_set_state = 0;
	in_flush = false;
	in_partition = false;
	subcnt = 0;
	_read_data_count = 0;
	_frozen = other->_frozen;
	
	layer_op_counter = other->layer_op_counter;
	freeze_length = other->freeze_length;
}

Playlist::Playlist (boost::shared_ptr<const Playlist> other, nframes_t start, nframes_t cnt, string str, bool hide)
	: _name (str), _session (other->_session), _orig_diskstream_id(other->_orig_diskstream_id)
{
	RegionLock rlock2 (const_cast<Playlist*> (other.get()));

	nframes_t end = start + cnt - 1;

	init (hide);

	in_set_state++;

	for (RegionList::const_iterator i = other->regions.begin(); i != other->regions.end(); i++) {

		boost::shared_ptr<Region> region;
		boost::shared_ptr<Region> new_region;
		nframes_t offset = 0;
		nframes_t position = 0;
		nframes_t len = 0;
		string    new_name;
		OverlapType overlap;

		region = *i;

		overlap = region->coverage (start, end);

		switch (overlap) {
		case OverlapNone:
			continue;

		case OverlapInternal:
			offset = start - region->position();
			position = 0;
			len = cnt;
			break;

		case OverlapStart:
			offset = 0;
			position = region->position() - start;
			len = end - region->position();
			break;

		case OverlapEnd:
			offset = start - region->position();
			position = 0;
			len = region->length() - offset;
			break;

		case OverlapExternal:
			offset = 0;
			position = region->position() - start;
			len = region->length();
			break;
		}

		_session.region_name (new_name, region->name(), false);

		new_region = RegionFactory::RegionFactory::create (region, offset, len, new_name, region->layer(), region->flags());

		add_region_internal (new_region, position);
	}
	
	in_set_state--;

	/* this constructor does NOT notify others (session) */
}

void
Playlist::use ()
{
	++_refcnt;
	InUse (true); /* EMIT SIGNAL */
}

void
Playlist::release ()
{
	if (_refcnt > 0) {
		_refcnt--; 
	}

	if (_refcnt == 0) {
		InUse (false); /* EMIT SIGNAL */
	}
}

void
Playlist::copy_regions (RegionList& newlist) const
{
	RegionLock rlock (const_cast<Playlist *> (this));

	for (RegionList::const_iterator i = regions.begin(); i != regions.end(); ++i) {
		newlist.push_back (RegionFactory::RegionFactory::create (*i));
	}
}

void
Playlist::init (bool hide)
{
	g_atomic_int_set (&block_notifications, 0);
	g_atomic_int_set (&ignore_state_changes, 0);
	pending_modified = false;
	pending_length = false;
	_refcnt = 0;
	_hidden = hide;
	_splicing = false;
	_nudging = false;
	in_set_state = 0;
	_edit_mode = Config->get_edit_mode();
	in_flush = false;
	in_partition = false;
	subcnt = 0;
	_read_data_count = 0;
	_frozen = false;
	layer_op_counter = 0;
	freeze_length = 0;

	Modified.connect (mem_fun (*this, &Playlist::mark_session_dirty));
}

Playlist::Playlist (const Playlist& pl)
	: _session (pl._session)
{
	fatal << _("playlist const copy constructor called") << endmsg;
}

Playlist::Playlist (Playlist& pl)
	: _session (pl._session)
{
	fatal << _("playlist non-const copy constructor called") << endmsg;
}

Playlist::~Playlist ()
{
	{ 
		RegionLock rl (this);

		for (set<boost::shared_ptr<Region> >::iterator i = all_regions.begin(); i != all_regions.end(); ++i) {
			(*i)->set_playlist (boost::shared_ptr<Playlist>());
		}
	}

	/* GoingAway must be emitted by derived classes */
}

void
Playlist::set_name (string str)
{
	/* in a typical situation, a playlist is being used
	   by one diskstream and also is referenced by the
	   Session. if there are more references than that,
	   then don't change the name.
	*/

	if (_refcnt > 2) {
		return;
	}

	_name = str; 
	NameChanged(); /* EMIT SIGNAL */
}

/***********************************************************************
 CHANGE NOTIFICATION HANDLING
 
 Notifications must be delayed till the region_lock is released. This
 is necessary because handlers for the signals may need to acquire
 the lock (e.g. to read from the playlist).
 ***********************************************************************/

void
Playlist::freeze ()
{
	delay_notifications ();
	g_atomic_int_inc (&ignore_state_changes);
}

void
Playlist::thaw ()
{
	g_atomic_int_dec_and_test (&ignore_state_changes);
	release_notifications ();
}


void
Playlist::delay_notifications ()
{
	g_atomic_int_inc (&block_notifications);
	freeze_length = _get_maximum_extent();
}

void
Playlist::release_notifications ()
{
	if (g_atomic_int_dec_and_test (&block_notifications)) { 
		flush_notifications ();
	} 
}


void
Playlist::notify_modified ()
{
	if (holding_state ()) {
		pending_modified = true;
	} else {
		pending_modified = false;
		Modified(); /* EMIT SIGNAL */
	}
}

void
Playlist::notify_region_removed (boost::shared_ptr<Region> r)
{
	if (holding_state ()) {
		pending_removes.insert (r);
		pending_modified = true;
		pending_length = true;
	} else {
		/* this might not be true, but we have to act
		   as though it could be.
		*/
		LengthChanged (); /* EMIT SIGNAL */
		Modified (); /* EMIT SIGNAL */
	}
}

void
Playlist::notify_region_added (boost::shared_ptr<Region> r)
{
	/* the length change might not be true, but we have to act
	   as though it could be.
	*/

	if (holding_state()) {
		pending_adds.insert (r);
		pending_modified = true;
		pending_length = true;
	} else {
		LengthChanged (); /* EMIT SIGNAL */
		Modified (); /* EMIT SIGNAL */
	}
}

void
Playlist::notify_length_changed ()
{
	if (holding_state ()) {
		pending_length = true;
	} else {
		LengthChanged(); /* EMIT SIGNAL */
		Modified (); /* EMIT SIGNAL */
	}
}

void
Playlist::flush_notifications ()
{
	set<boost::shared_ptr<Region> > dependent_checks_needed;
	set<boost::shared_ptr<Region> >::iterator s;
	uint32_t n = 0;

	if (in_flush) {
		return;
	}

	in_flush = true;

	/* we have no idea what order the regions ended up in pending
	   bounds (it could be based on selection order, for example).
	   so, to preserve layering in the "most recently moved is higher" 
	   model, sort them by existing layer, then timestamp them.
	*/

	// RegionSortByLayer cmp;
	// pending_bounds.sort (cmp);

	for (RegionList::iterator r = pending_bounds.begin(); r != pending_bounds.end(); ++r) {
		if (Config->get_layer_model() == MoveAddHigher) {
			timestamp_layer_op (*r);
		}
		pending_length = true;
		dependent_checks_needed.insert (*r);
		n++;
	}

	for (s = pending_adds.begin(); s != pending_adds.end(); ++s) {
		dependent_checks_needed.insert (*s);
		n++;
	}

	for (s = pending_removes.begin(); s != pending_removes.end(); ++s) {
		remove_dependents (*s);
		n++;
	}

	if ((freeze_length != _get_maximum_extent()) || pending_length) {
		pending_length = 0;
		LengthChanged(); /* EMIT SIGNAL */
		n++;
	}

	if (n || pending_modified) {
		if (!in_set_state) {
			possibly_splice ();
			relayer ();
		}
		pending_modified = false;
		Modified (); /* EMIT SIGNAL */
	}

	for (s = dependent_checks_needed.begin(); s != dependent_checks_needed.end(); ++s) {
		check_dependents (*s, false);
	}

	pending_adds.clear ();
	pending_removes.clear ();
	pending_bounds.clear ();

	in_flush = false;
}

/*************************************************************
  PLAYLIST OPERATIONS
 *************************************************************/

void
Playlist::add_region (boost::shared_ptr<Region> region, nframes_t position, float times) 
{ 
	RegionLock rlock (this);
	
	times = fabs (times);
	
	int itimes = (int) floor (times);

	nframes_t pos = position;
	
	if (itimes >= 1) {
		add_region_internal (region, pos);
		pos += region->length();
		--itimes;
	}
	
	/* later regions will all be spliced anyway */
	
	if (!holding_state ()) {
		possibly_splice_unlocked ();
	}

	/* note that itimes can be zero if we being asked to just
	   insert a single fraction of the region.
	*/

	for (int i = 0; i < itimes; ++i) {
		boost::shared_ptr<Region> copy = RegionFactory::create (region);
		add_region_internal (copy, pos);
		pos += region->length();
	}
	
	if (floor (times) != times) {
		nframes_t length = (nframes_t) floor (region->length() * (times - floor (times)));
		string name;
		_session.region_name (name, region->name(), false);
		boost::shared_ptr<Region> sub = RegionFactory::create (region, 0, length, name, region->layer(), region->flags());
		add_region_internal (sub, pos);
	}
}

void
Playlist::set_region_ownership ()
{
	RegionLock rl (this);
	RegionList::iterator i;
	boost::weak_ptr<Playlist> pl (shared_from_this());

	for (i = regions.begin(); i != regions.end(); ++i) {
		(*i)->set_playlist (pl);
	}
}

void
Playlist::add_region_internal (boost::shared_ptr<Region> region, nframes_t position)
{
	RegionSortByPosition cmp;
	nframes_t old_length = 0;

	if (!holding_state()) {
		 old_length = _get_maximum_extent();
	}

	if (!in_set_state) {
		boost::shared_ptr<Playlist> foo (shared_from_this());
		region->set_playlist (boost::weak_ptr<Playlist>(foo));
	}

	region->set_position (position, this);

	timestamp_layer_op (region);

	regions.insert (upper_bound (regions.begin(), regions.end(), region, cmp), region);
	all_regions.insert (region);

	if (!holding_state () && !in_set_state) {
		/* layers get assigned from XML state */
		relayer ();
	}

	/* we need to notify the existence of new region before checking dependents. Ick. */

	notify_region_added (region);
	
	if (!holding_state ()) {
		check_dependents (region, false);
		if (old_length != _get_maximum_extent()) {
			notify_length_changed ();
		}
	}

	region->StateChanged.connect (sigc::bind (mem_fun (this, &Playlist::region_changed_proxy), 
						  boost::weak_ptr<Region> (region)));
}

void
Playlist::replace_region (boost::shared_ptr<Region> old, boost::shared_ptr<Region> newr, nframes_t pos)
{
	RegionLock rlock (this);

	remove_region_internal (old);
	add_region_internal (newr, pos);

	if (!holding_state ()) {
		possibly_splice_unlocked ();
	}
}

void
Playlist::remove_region (boost::shared_ptr<Region> region)
{
	RegionLock rlock (this);
	remove_region_internal (region);

	if (!holding_state ()) {
		possibly_splice_unlocked ();
	}
}

int
Playlist::remove_region_internal (boost::shared_ptr<Region>region)
{
	RegionList::iterator i;
	nframes_t old_length = 0;

	if (!holding_state()) {
		old_length = _get_maximum_extent();
	}

	for (i = regions.begin(); i != regions.end(); ++i) {
		if (*i == region) {

			regions.erase (i);

			if (!holding_state ()) {
				relayer ();
				remove_dependents (region);
				
				if (old_length != _get_maximum_extent()) {
					notify_length_changed ();
				}
			}

			notify_region_removed (region);
			return 0;
		}
	}
	return -1;
}

void
Playlist::get_equivalent_regions (boost::shared_ptr<Region> other, vector<boost::shared_ptr<Region> >& results)
{
	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
		if (Config->get_use_overlap_equivalency()) {
			if ((*i)->overlap_equivalent (other)) {
				results.push_back ((*i));
			} else if ((*i)->equivalent (other)) {
				results.push_back ((*i));
			}
		}
	}
}

void
Playlist::get_region_list_equivalent_regions (boost::shared_ptr<Region> other, vector<boost::shared_ptr<Region> >& results)
{
	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {

		if ((*i) && (*i)->region_list_equivalent (other)) {
			results.push_back (*i);
		}
	}
}

void
Playlist::partition (nframes_t start, nframes_t end, bool just_top_level)
{
	RegionList thawlist;

	partition_internal (start, end, false, thawlist);

	for (RegionList::iterator i = thawlist.begin(); i != thawlist.end(); ++i) {
		(*i)->thaw ("separation");
	}
}

void
Playlist::partition_internal (nframes_t start, nframes_t end, bool cutting, RegionList& thawlist)
{
	RegionLock rlock (this);
	boost::shared_ptr<Region> region;
	boost::shared_ptr<Region> current;
	string new_name;
	RegionList::iterator tmp;
	OverlapType overlap;
	nframes_t pos1, pos2, pos3, pos4;
	RegionList new_regions;

	in_partition = true;

	/* need to work from a copy, because otherwise the regions we add during the process
	   get operated on as well.
	*/

	RegionList copy = regions;

	for (RegionList::iterator i = copy.begin(); i != copy.end(); i = tmp) {
		
		tmp = i;
		++tmp;

		current = *i;
		
		if (current->first_frame() == start && current->last_frame() == end) {
			if (cutting) {
				remove_region_internal (current);
			}
			continue;
		}
		
		if ((overlap = current->coverage (start, end)) == OverlapNone) {
			continue;
		}
		
		pos1 = current->position();
		pos2 = start;
		pos3 = end;
		pos4 = current->last_frame();

		if (overlap == OverlapInternal) {
			
			/* split: we need 3 new regions, the front, middle and end.
			   cut:   we need 2 regions, the front and end.
			*/
			
			/*
			                 start                 end
			  ---------------*************************------------
			                 P1  P2              P3  P4
			  SPLIT:
			  ---------------*****++++++++++++++++====------------
			  CUT
			  ---------------*****----------------====------------
			  
			*/

			if (!cutting) {
				
				/* "middle" ++++++ */
				
				_session.region_name (new_name, current->name(), false);
				region = RegionFactory::create (current, pos2 - pos1, pos3 - pos2, new_name,
						       regions.size(), Region::Flag(current->flags()|Region::Automatic|Region::LeftOfSplit|Region::RightOfSplit));
				add_region_internal (region, start);
				new_regions.push_back (region);
			}
			
			/* "end" ====== */
			
			_session.region_name (new_name, current->name(), false);
			region = RegionFactory::create (current, pos3 - pos1, pos4 - pos3, new_name, 
					       regions.size(), Region::Flag(current->flags()|Region::Automatic|Region::RightOfSplit));

			add_region_internal (region, end);
			new_regions.push_back (region);

		        /* "front" ***** */
				
			current->freeze ();
			thawlist.push_back (current);
			current->trim_end (pos2, this);

		} else if (overlap == OverlapEnd) {

			/*
				                              start           end
				    ---------------*************************------------
				                   P1           P2         P4   P3
                                    SPLIT:						   
				    ---------------**************+++++++++++------------
                                    CUT:						   
				    ---------------**************-----------------------

			*/

			if (!cutting) {
				
				/* end +++++ */
				
				_session.region_name (new_name, current->name(), false);
				region = RegionFactory::create (current, pos2 - pos1, pos4 - pos2, new_name, (layer_t) regions.size(),
						       Region::Flag(current->flags()|Region::Automatic|Region::LeftOfSplit));
				add_region_internal (region, start);
				new_regions.push_back (region);
			}

			/* front ****** */

			current->freeze ();
			thawlist.push_back (current);
			current->trim_end (pos2, this);

		} else if (overlap == OverlapStart) {

			/* split: we need 2 regions: the front and the end.
			   cut: just trim current to skip the cut area
			*/
				
			/*
				                        start           end
				    ---------------*************************------------
				       P2          P1 P3                   P4          

				    SPLIT:
				    ---------------****+++++++++++++++++++++------------
				    CUT:
				    -------------------*********************------------
				    
			*/

			if (!cutting) {
				
				/* front **** */
				 _session.region_name (new_name, current->name(), false);
				 region = RegionFactory::create (current, 0, pos3 - pos1, new_name,
							regions.size(), Region::Flag(current->flags()|Region::Automatic|Region::RightOfSplit));
				 add_region_internal (region, pos1);
				 new_regions.push_back (region);
			} 
			
			/* end */
			
			current->freeze ();
			thawlist.push_back (current);
			current->trim_front (pos3, this);

		} else if (overlap == OverlapExternal) {

			/* split: no split required.
			   cut: remove the region.
			*/
				
			/*
				       start                                      end
				    ---------------*************************------------
				       P2          P1 P3                   P4          

				    SPLIT:
				    ---------------*************************------------
				    CUT:
				    ----------------------------------------------------
				    
			*/

			if (cutting) {
				remove_region_internal (current);
			}
			new_regions.push_back (current);
		}
	}

	in_partition = false;

	for (RegionList::iterator i = new_regions.begin(); i != new_regions.end(); ++i) {
		check_dependents (*i, false);
	}
}

boost::shared_ptr<Playlist>
Playlist::cut_copy (boost::shared_ptr<Playlist> (Playlist::*pmf)(nframes_t, nframes_t,bool), list<AudioRange>& ranges, bool result_is_hidden)
{
	boost::shared_ptr<Playlist> ret;
	boost::shared_ptr<Playlist> pl;
	nframes_t start;

	if (ranges.empty()) {
		return boost::shared_ptr<Playlist>();
	}

	start = ranges.front().start;

	for (list<AudioRange>::iterator i = ranges.begin(); i != ranges.end(); ++i) {

		pl = (this->*pmf)((*i).start, (*i).length(), result_is_hidden);
		
		if (i == ranges.begin()) {
			ret = pl;
		} else {
			
			/* paste the next section into the nascent playlist,
			   offset to reflect the start of the first range we
			   chopped.
			*/

			ret->paste (pl, (*i).start - start, 1.0f);
		}
	}

	return ret;
}

boost::shared_ptr<Playlist>
Playlist::cut (list<AudioRange>& ranges, bool result_is_hidden)
{
	boost::shared_ptr<Playlist> (Playlist::*pmf)(nframes_t,nframes_t,bool) = &Playlist::cut;
	return cut_copy (pmf, ranges, result_is_hidden);
}

boost::shared_ptr<Playlist>
Playlist::copy (list<AudioRange>& ranges, bool result_is_hidden)
{
	boost::shared_ptr<Playlist> (Playlist::*pmf)(nframes_t,nframes_t,bool) = &Playlist::copy;
	return cut_copy (pmf, ranges, result_is_hidden);
}

boost::shared_ptr<Playlist>
Playlist::cut (nframes_t start, nframes_t cnt, bool result_is_hidden)
{
	boost::shared_ptr<Playlist> the_copy;
	RegionList thawlist;
	char buf[32];

	snprintf (buf, sizeof (buf), "%" PRIu32, ++subcnt);
	string new_name = _name;
	new_name += '.';
	new_name += buf;

	if ((the_copy = PlaylistFactory::create (shared_from_this(), start, cnt, new_name, result_is_hidden)) == 0) {
		return boost::shared_ptr<Playlist>();
	}

	partition_internal (start, start+cnt-1, true, thawlist);
	possibly_splice ();

	for (RegionList::iterator i = thawlist.begin(); i != thawlist.end(); ++i) {
		(*i)->thaw ("playlist cut");
	}

	return the_copy;
}

boost::shared_ptr<Playlist>
Playlist::copy (nframes_t start, nframes_t cnt, bool result_is_hidden)
{
	char buf[32];
	
	snprintf (buf, sizeof (buf), "%" PRIu32, ++subcnt);
	string new_name = _name;
	new_name += '.';
	new_name += buf;

	cnt = min (_get_maximum_extent() - start, cnt);
	return PlaylistFactory::create (shared_from_this(), start, cnt, new_name, result_is_hidden);
}

int
Playlist::paste (boost::shared_ptr<Playlist> other, nframes_t position, float times)
{
	times = fabs (times);
	nframes_t old_length;

	{
		RegionLock rl1 (this);
		RegionLock rl2 (other.get());

		old_length = _get_maximum_extent();
	
		int itimes = (int) floor (times);
		nframes_t pos = position;
		nframes_t shift = other->_get_maximum_extent();
		layer_t top_layer = regions.size();

		while (itimes--) {
			for (RegionList::iterator i = other->regions.begin(); i != other->regions.end(); ++i) {
				boost::shared_ptr<Region> copy_of_region = RegionFactory::create (*i);

				/* put these new regions on top of all existing ones, but preserve
				   the ordering they had in the original playlist.
				*/
				
				copy_of_region->set_layer (copy_of_region->layer() + top_layer);
				add_region_internal (copy_of_region, copy_of_region->position() + pos);
			}
			pos += shift;
		}

		possibly_splice_unlocked ();

		/* XXX shall we handle fractional cases at some point? */

		if (old_length != _get_maximum_extent()) {
			notify_length_changed ();
		}

		
	}

	return 0;
}


void
Playlist::duplicate (boost::shared_ptr<Region> region, nframes_t position, float times)
{
	times = fabs (times);

	RegionLock rl (this);
	int itimes = (int) floor (times);
	nframes_t pos = position;

	while (itimes--) {
		boost::shared_ptr<Region> copy = RegionFactory::create (region);
		add_region_internal (copy, pos);
		pos += region->length();
	}

	if (floor (times) != times) {
		nframes_t length = (nframes_t) floor (region->length() * (times - floor (times)));
		string name;
		_session.region_name (name, region->name(), false);
		boost::shared_ptr<Region> sub = RegionFactory::create (region, 0, length, name, region->layer(), region->flags());
		add_region_internal (sub, pos);
	}
}

void
Playlist::split_region (boost::shared_ptr<Region> region, nframes_t playlist_position)
{
	RegionLock rl (this);

	if (!region->covers (playlist_position)) {
		return;
	}

	if (region->position() == playlist_position ||
	    region->last_frame() == playlist_position) {
		return;
	}

	boost::shared_ptr<Region> left;
	boost::shared_ptr<Region> right;
	nframes_t before;
	nframes_t after;
	string before_name;
	string after_name;

	before = playlist_position - region->position();
	after = region->length() - before;
	
	
	_session.region_name (before_name, region->name(), false);
	left = RegionFactory::create (region, 0, before, before_name, region->layer(), Region::Flag (region->flags()|Region::LeftOfSplit));

	_session.region_name (after_name, region->name(), false);
	right = RegionFactory::create (region, before, after, after_name, region->layer(), Region::Flag (region->flags()|Region::RightOfSplit));

	add_region_internal (left, region->position());
	add_region_internal (right, region->position() + before);
	
	uint64_t orig_layer_op = region->last_layer_op();
	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
		if ((*i)->last_layer_op() > orig_layer_op) {
			(*i)->set_last_layer_op( (*i)->last_layer_op() + 1 );
		}
	}
	
	left->set_last_layer_op ( orig_layer_op );
	right->set_last_layer_op ( orig_layer_op + 1);

	layer_op_counter++;

	finalize_split_region (region, left, right);
	
	if (remove_region_internal (region)) {
		return;
	}
}

void
Playlist::possibly_splice ()
{
	if (_edit_mode == Splice) {
		splice_locked ();
	}
}

void
Playlist::possibly_splice_unlocked ()
{
	if (_edit_mode == Splice) {
		splice_unlocked ();
	}
}

void
Playlist::splice_locked ()
{
	{
		RegionLock rl (this);
		core_splice ();
	}

	notify_length_changed ();
}

void
Playlist::splice_unlocked ()
{
	core_splice ();
	notify_length_changed ();
}

void
Playlist::core_splice ()
{
	_splicing = true;
	
	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
		
		RegionList::iterator next;
		
		next = i;
		++next;
		
		if (next == regions.end()) {
			break;
		}
		
		(*next)->set_position ((*i)->last_frame() + 1, this);
	}
	
	_splicing = false;
}

void
Playlist::region_bounds_changed (Change what_changed, boost::shared_ptr<Region> region)
{
	if (in_set_state || _splicing || _nudging) {
		return;
	}

	if (what_changed & ARDOUR::PositionChanged) {

		/* remove it from the list then add it back in
		   the right place again.
		*/
		
		RegionSortByPosition cmp;

		RegionList::iterator i = find (regions.begin(), regions.end(), region);
		
		if (i == regions.end()) {
			warning << string_compose (_("%1: bounds changed received for region (%2)not in playlist"),
					    _name, region->name())
				<< endmsg;
			return;
		}

		regions.erase (i);
		regions.insert (upper_bound (regions.begin(), regions.end(), region, cmp), region);

	}

	if (what_changed & Change (ARDOUR::PositionChanged|ARDOUR::LengthChanged)) {
		if (holding_state ()) {
			pending_bounds.push_back (region);
		} else {
			if (Config->get_layer_model() == MoveAddHigher) {
				/* it moved or changed length, so change the timestamp */
				timestamp_layer_op (region);
			}
			
			possibly_splice ();
			notify_length_changed ();
			relayer ();
			check_dependents (region, false);
		}
	}
}

void
Playlist::region_changed_proxy (Change what_changed, boost::weak_ptr<Region> weak_region)
{
	boost::shared_ptr<Region> region (weak_region.lock());

	if (!region) {
		return;
	}


	/* this makes a virtual call to the right kind of playlist ... */

	region_changed (what_changed, region);
}

bool
Playlist::region_changed (Change what_changed, boost::shared_ptr<Region> region)
{
	Change our_interests = Change (Region::MuteChanged|Region::LayerChanged|Region::OpacityChanged);
	bool save = false;

	if (in_set_state || in_flush) {
		return false;
	}

	{
		if (what_changed & BoundsChanged) {
			region_bounds_changed (what_changed, region);
			save = !(_splicing || _nudging);
		}
		
		if ((what_changed & Region::MuteChanged) && 
		    !(what_changed &  Change (ARDOUR::PositionChanged|ARDOUR::LengthChanged))) {
			check_dependents (region, false);
		}
		
		if (what_changed & our_interests) {
			save = true;
		}
	}

	return save;
}

void
Playlist::clear (bool with_signals)
{
	{ 
		RegionLock rl (this);
		for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
			pending_removes.insert (*i);
		}
		regions.clear ();
	}

	if (with_signals) {
		LengthChanged ();
		Modified ();
	}

}

/***********************************************************************
 FINDING THINGS
 **********************************************************************/

Playlist::RegionList *
Playlist::regions_at (nframes_t frame)

{
	RegionLock rlock (this);
	return find_regions_at (frame);
}	

boost::shared_ptr<Region>
Playlist::top_region_at (nframes_t frame)

{
	RegionLock rlock (this);
	RegionList *rlist = find_regions_at (frame);
	boost::shared_ptr<Region> region;
	
	if (rlist->size()) {
		RegionSortByLayer cmp;
		rlist->sort (cmp);
		region = rlist->back();
	} 

	delete rlist;
	return region;
}	

Playlist::RegionList *
Playlist::find_regions_at (nframes_t frame)
{
	RegionList *rlist = new RegionList;

	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
		if ((*i)->covers (frame)) {
			rlist->push_back (*i);
		}
	}

	return rlist;
}

Playlist::RegionList *
Playlist::regions_touched (nframes_t start, nframes_t end)
{
	RegionLock rlock (this);
	RegionList *rlist = new RegionList;

	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
		if ((*i)->coverage (start, end) != OverlapNone) {
			rlist->push_back (*i);
		}
	}

	return rlist;
}


boost::shared_ptr<Region>
Playlist::find_next_region (nframes_t frame, RegionPoint point, int dir)
{
	RegionLock rlock (this);
	boost::shared_ptr<Region> ret;
	nframes_t closest = max_frames;


	for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {

		nframes_t distance;
		boost::shared_ptr<Region> r = (*i);
		nframes_t pos = 0;

		switch (point) {
		case Start:
			pos = r->first_frame ();
			break;
		case End:
			pos = r->last_frame ();
			break;
		case SyncPoint:
			pos = r->adjust_to_sync (r->first_frame());
			break;
		}

		switch (dir) {
		case 1: /* forwards */

			if (pos >= frame) {
				if ((distance = pos - frame) < closest) {
					closest = distance;
					ret = r;
				}
			}

			break;

		default: /* backwards */

			if (pos <= frame) {
				if ((distance = frame - pos) < closest) {
					closest = distance;
					ret = r;
				}
			}
			break;
		}
	}

	return ret;
}

/***********************************************************************/



void
Playlist::mark_session_dirty ()
{
	if (!in_set_state && !holding_state ()) {
		_session.set_dirty();
	}
}

int
Playlist::set_state (const XMLNode& node)
{
	XMLNode *child;
	XMLNodeList nlist;
	XMLNodeConstIterator niter;
	XMLPropertyList plist;
	XMLPropertyConstIterator piter;
	XMLProperty *prop;
	boost::shared_ptr<Region> region;
	string region_name;

	in_set_state++;

	if (node.name() != "Playlist") {
		in_set_state--;
		return -1;
	}

	freeze ();

	plist = node.properties();

	for (piter = plist.begin(); piter != plist.end(); ++piter) {

		prop = *piter;
		
		if (prop->name() == X_("name")) {
			_name = prop->value();
		} else if (prop->name() == X_("orig_diskstream_id")) {
			_orig_diskstream_id = prop->value ();
		} else if (prop->name() == X_("frozen")) {
			_frozen = (prop->value() == X_("yes"));
		}
	}

	clear (false);
	
	nlist = node.children();

	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {

		child = *niter;
		
		if (child->name() == "Region") {

			if ((prop = child->property ("id")) == 0) {
				error << _("region state node has no ID, ignored") << endmsg;
				continue;
			}
			
			ID id = prop->value ();
			
			if ((region = region_by_id (id))) {

				Change what_changed = Change (0);

				if (region->set_live_state (*child, what_changed, true)) {
					error << _("Playlist: cannot reset region state from XML") << endmsg;
					continue;
				}

			} else if ((region = RegionFactory::create (_session, *child, true)) == 0) {
				error << _("Playlist: cannot create region from XML") << endmsg;
				continue;
			}

			add_region (region, region->position(), 1.0);

			// So that layer_op ordering doesn't get screwed up
			region->set_last_layer_op( region->layer());

		} 			
	}
	
	notify_modified ();

	thaw ();

 	/* update dependents, which was not done during add_region_internal 
	   due to in_set_state being true 
	*/

	for (RegionList::iterator r = regions.begin(); r != regions.end(); ++r) {
		check_dependents (*r, false);
	}

	in_set_state--;

	return 0;
}

XMLNode&
Playlist::get_state()
{
	return state(true);
}

XMLNode&
Playlist::get_template()
{
	return state(false);
}

XMLNode&
Playlist::state (bool full_state)
{
	XMLNode *node = new XMLNode (X_("Playlist"));
	char buf[64];
	
	node->add_property (X_("name"), _name);

	_orig_diskstream_id.print (buf, sizeof (buf));
	node->add_property (X_("orig_diskstream_id"), buf);
	node->add_property (X_("frozen"), _frozen ? "yes" : "no");

	if (full_state) {
		RegionLock rlock (this, false);
		for (RegionList::iterator i = regions.begin(); i != regions.end(); ++i) {
			node->add_child_nocopy ((*i)->get_state());
		}
	}

	if (_extra_xml) {
		node->add_child_copy (*_extra_xml);
	}

	return *node;
}

bool
Playlist::empty() const
{
	RegionLock rlock (const_cast<Playlist *>(this), false);
	return regions.empty();
}

uint32_t
Playlist::n_regions() const
{
	RegionLock rlock (const_cast<Playlist *>(this), false);
	return regions.size();
}

nframes_t
Playlist::get_maximum_extent () const
{
	RegionLock rlock (const_cast<Playlist *>(this), false);
	return _get_maximum_extent ();
}

nframes_t
Playlist::_get_maximum_extent () const
{
	RegionList::const_iterator i;
	nframes_t max_extent = 0;
	nframes_t end = 0;

	for (i = regions.begin(); i != regions.end(); ++i) {
		if ((end = (*i)->position() + (*i)->length()) > max_extent) {
			max_extent = end;
		}
	}

	return max_extent;
}

string 
Playlist::bump_name (string name, Session &session)
{
	string newname = name;

	do {
		newname = Playlist::bump_name_once (newname);
	} while (session.playlist_by_name (newname)!=NULL);

	return newname;
}

string
Playlist::bump_name_once (string name)
{
	string::size_type period;
	string newname;

	if ((period = name.find_last_of ('.')) == string::npos) {
		newname = name;
		newname += ".1";
	} else {
		char buf[32];
		int version;
		
		sscanf (name.substr (period+1).c_str(), "%d", &version);
		snprintf (buf, sizeof(buf), "%d", version+1);
		
		newname = name.substr (0, period+1);
		newname += buf;
	}

	return newname;
}

layer_t
Playlist::top_layer() const
{
	RegionLock rlock (const_cast<Playlist *> (this));
	layer_t top = 0;

	for (RegionList::const_iterator i = regions.begin(); i != regions.end(); ++i) {
		top = max (top, (*i)->layer());
	}
	return top;
}

void
Playlist::set_edit_mode (EditMode mode)
{
	_edit_mode = mode;
}

/********************
 * Region Layering
 ********************/

void
Playlist::relayer ()
{
	RegionList::iterator i;
	uint32_t layer = 0;

	/* don't send multiple Modified notifications
	   when multiple regions are relayered.
	*/

	freeze ();

	if (Config->get_layer_model() == MoveAddHigher || 
	    Config->get_layer_model() == AddHigher) {

		RegionSortByLastLayerOp cmp;
		RegionList copy = regions;

		copy.sort (cmp);

		for (i = copy.begin(); i != copy.end(); ++i) {
			(*i)->set_layer (layer++);
		}

	} else {
		
		/* Session::LaterHigher model */

		for (i = regions.begin(); i != regions.end(); ++i) {
			(*i)->set_layer (layer++);
		}
	}

	/* sending Modified means that various kinds of layering
	   models operate correctly at the GUI
	   level. slightly inefficient, but only slightly.

	   We force a Modified signal here in case no layers actually
	   changed.
	*/

	notify_modified ();

	thaw ();
}

/* XXX these layer functions are all deprecated */

void
Playlist::raise_region (boost::shared_ptr<Region> region)
{
	uint32_t rsz = regions.size();
	layer_t target = region->layer() + 1U;

	if (target >= rsz) {
		/* its already at the effective top */
		return;
	}

	move_region_to_layer (target, region, 1);
}

void
Playlist::lower_region (boost::shared_ptr<Region> region)
{
	if (region->layer() == 0) {
		/* its already at the bottom */
		return;
	}

	layer_t target = region->layer() - 1U;

	move_region_to_layer (target, region, -1);
}

void
Playlist::raise_region_to_top (boost::shared_ptr<Region> region)
{
	/* does nothing useful if layering mode is later=higher */
	if ((Config->get_layer_model() == MoveAddHigher) ||
	    (Config->get_layer_model() == AddHigher)) {
		timestamp_layer_op (region);
		relayer ();
	}
}

void
Playlist::lower_region_to_bottom (boost::shared_ptr<Region> region)
{
	/* does nothing useful if layering mode is later=higher */
	if ((Config->get_layer_model() == MoveAddHigher) ||
	    (Config->get_layer_model() == AddHigher)) {
		region->set_last_layer_op (0);
		relayer ();
	}
}

int
Playlist::move_region_to_layer (layer_t target_layer, boost::shared_ptr<Region> region, int dir)
{
	RegionList::iterator i;
	typedef pair<boost::shared_ptr<Region>,layer_t> LayerInfo;
	list<LayerInfo> layerinfo;
	layer_t dest;

	{
		RegionLock rlock (const_cast<Playlist *> (this));
		
		for (i = regions.begin(); i != regions.end(); ++i) {
			
			if (region == *i) {
				continue;
			}

			if (dir > 0) {

				/* region is moving up, move all regions on intermediate layers
				   down 1
				*/
				
				if ((*i)->layer() > region->layer() && (*i)->layer() <= target_layer) {
					dest = (*i)->layer() - 1;
				} else {
					/* not affected */
					continue;
				}
			} else {

				/* region is moving down, move all regions on intermediate layers
				   up 1
				*/

				if ((*i)->layer() < region->layer() && (*i)->layer() >= target_layer) {
					dest = (*i)->layer() + 1;
				} else {
					/* not affected */
					continue;
				}
			}

			LayerInfo newpair;
			
			newpair.first = *i;
			newpair.second = dest;
			
			layerinfo.push_back (newpair);
		} 
	}

	/* now reset the layers without holding the region lock */

	for (list<LayerInfo>::iterator x = layerinfo.begin(); x != layerinfo.end(); ++x) {
		x->first->set_layer (x->second);
	}

	region->set_layer (target_layer);

	/* now check all dependents */

	for (list<LayerInfo>::iterator x = layerinfo.begin(); x != layerinfo.end(); ++x) {
		check_dependents (x->first, false);
	}
	
	check_dependents (region, false);
	
	return 0;
}

void
Playlist::nudge_after (nframes_t start, nframes_t distance, bool forwards)
{
	RegionList::iterator i;
	nframes_t new_pos;
	bool moved = false;

	_nudging = true;

	{
		RegionLock rlock (const_cast<Playlist *> (this));
		
		for (i = regions.begin(); i != regions.end(); ++i) {

			if ((*i)->position() >= start) {

				if (forwards) {

					if ((*i)->last_frame() > max_frames - distance) {
						new_pos = max_frames - (*i)->length();
					} else {
						new_pos = (*i)->position() + distance;
					}
					
				} else {
					
					if ((*i)->position() > distance) {
						new_pos = (*i)->position() - distance;
					} else {
						new_pos = 0;
					}
				}

				(*i)->set_position (new_pos, this);
				moved = true;
			}
		}
	}

	if (moved) {
		_nudging = false;
		notify_length_changed ();
	}

}

boost::shared_ptr<Region>
Playlist::find_region (const ID& id) const
{
	RegionLock rlock (const_cast<Playlist*> (this));

	/* searches all regions currently in use by the playlist */

	for (RegionList::const_iterator i = regions.begin(); i != regions.end(); ++i) {
		if ((*i)->id() == id) {
			return *i;
		}
	}

	return boost::shared_ptr<Region> ();
}

boost::shared_ptr<Region>
Playlist::region_by_id (ID id)
{
	/* searches all regions ever added to this playlist */

	for (set<boost::shared_ptr<Region> >::iterator i = all_regions.begin(); i != all_regions.end(); ++i) {
		if ((*i)->id() == id) {
			return *i;
		}
	}
	return boost::shared_ptr<Region> ();
}
	
void
Playlist::dump () const
{
	boost::shared_ptr<Region> r;

	cerr << "Playlist \"" << _name << "\" " << endl
	     << regions.size() << " regions "
	     << endl;

	for (RegionList::const_iterator i = regions.begin(); i != regions.end(); ++i) {
		r = *i;
		cerr << "  " << r->name() << " [" 
		     << r->start() << "+" << r->length() 
		     << "] at " 
		     << r->position()
		     << " on layer "
		     << r->layer ()
		     << endl;
	}
}

void
Playlist::set_frozen (bool yn)
{
	_frozen = yn;
}

void
Playlist::timestamp_layer_op (boost::shared_ptr<Region> region)
{
//	struct timeval tv;
//	gettimeofday (&tv, 0);
	region->set_last_layer_op (++layer_op_counter);
}

