/*
    Copyright (C) 2000 Paul Davis 

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

#ifndef __ardour_overlap_h__
#define __ardour_overlap_h__

#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sigc++/signal.h>

#include <pbd/undo.h>
#include <pbd/statefuldestructible.h> 

#include <ardour/ardour.h>
#include <ardour/curve.h>
#include <ardour/audioregion.h>
#include <ardour/crossfade_compare.h>

namespace ARDOUR {

class AudioRegion;
class Playlist;

class Crossfade : public PBD::StatefulDestructible, public boost::enable_shared_from_this<ARDOUR::Crossfade>
{
  public:

	class NoCrossfadeHere: std::exception {
	  public:
		virtual const char *what() const throw() { return "no crossfade should be constructed here"; }
	};
	
	/* constructor for "fixed" xfades at each end of an internal overlap */

	Crossfade (boost::shared_ptr<ARDOUR::AudioRegion> in, boost::shared_ptr<ARDOUR::AudioRegion> out,
		   nframes_t position,
		   nframes_t initial_length,
		   AnchorPoint);

	/* constructor for xfade between two regions that are overlapped in any way
	   except the "internal" case.
	*/
	
	Crossfade (boost::shared_ptr<ARDOUR::AudioRegion> in, boost::shared_ptr<ARDOUR::AudioRegion> out, CrossfadeModel, bool active);


	/* copy constructor to copy a crossfade with new regions. used (for example)
	   when a playlist copy is made 
	*/
	Crossfade (const Crossfade &, boost::shared_ptr<ARDOUR::AudioRegion>, boost::shared_ptr<ARDOUR::AudioRegion>);
	
	/* the usual XML constructor */

	Crossfade (const Playlist&, XMLNode&);
	virtual ~Crossfade();

	bool operator== (const ARDOUR::Crossfade&);

	XMLNode& get_state (void);
	int set_state (const XMLNode&);

	boost::shared_ptr<ARDOUR::AudioRegion> in() const { return _in; }
	boost::shared_ptr<ARDOUR::AudioRegion> out() const { return _out; }
	
	nframes_t read_at (Sample *buf, Sample *mixdown_buffer, 
				float *gain_buffer, nframes_t position, nframes_t cnt, 
				uint32_t chan_n,
				nframes_t read_frames = 0,
				nframes_t skip_frames = 0);
	
	bool refresh ();

	uint32_t upper_layer () const {
		return std::max (_in->layer(), _out->layer());
	}

	uint32_t lower_layer () const {
		return std::min (_in->layer(), _out->layer());
	}

	bool involves (boost::shared_ptr<ARDOUR::AudioRegion> region) const {
		return _in == region || _out == region;
	}

	bool involves (boost::shared_ptr<ARDOUR::AudioRegion> a, boost::shared_ptr<ARDOUR::AudioRegion> b) const {
		return (_in == a && _out == b) || (_in == b && _out == a);
	}

	nframes_t length() const { return _length; }
	nframes_t overlap_length() const;
	nframes_t position() const { return _position; }

	void invalidate();

	sigc::signal<void,boost::shared_ptr<Crossfade> > Invalidated;
	sigc::signal<void,Change>     StateChanged;

	bool covers (nframes_t frame) const {
		return _position <= frame && frame < _position + _length;
	}

	OverlapType coverage (nframes_t start, nframes_t end) const;

	static void set_buffer_size (nframes_t);

	bool active () const { return _active; }
	void set_active (bool yn);

	bool following_overlap() const { return _follow_overlap; }
	bool can_follow_overlap() const;
	void set_follow_overlap (bool yn);

	Curve& fade_in() { return _fade_in; } 
	Curve& fade_out() { return _fade_out; }

	nframes_t set_length (nframes_t);
	
	static nframes_t short_xfade_length() { return _short_xfade_length; }
	static void set_short_xfade_length (nframes_t n);

	static Change ActiveChanged;
	static Change FollowOverlapChanged;

  private:
	friend struct CrossfadeComparePtr;
	friend class AudioPlaylist;

	static nframes_t _short_xfade_length;

	boost::shared_ptr<ARDOUR::AudioRegion> _in;
	boost::shared_ptr<ARDOUR::AudioRegion> _out;
	bool                 _active;
	bool                 _in_update;
	OverlapType           overlap_type;
	nframes_t       _length;
	nframes_t       _position;
	AnchorPoint          _anchor_point;
	bool                 _follow_overlap;
	bool                 _fixed;
	int32_t               layer_relation;
	Curve _fade_in;
	Curve _fade_out;

	static Sample* crossfade_buffer_out;
	static Sample* crossfade_buffer_in;

	void initialize ();
	int  compute (boost::shared_ptr<ARDOUR::AudioRegion>, boost::shared_ptr<ARDOUR::AudioRegion>, CrossfadeModel);
	bool update ();
};


} // namespace ARDOUR

#endif /* __ardour_overlap_h__ */	
