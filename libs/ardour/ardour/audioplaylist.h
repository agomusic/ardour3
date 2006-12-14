/*
    Copyright (C) 2003 Paul Davis 

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

#ifndef __ardour_audio_playlist_h__
#define __ardour_audio_playlist_h__

#include <vector>
#include <list>

#include <ardour/ardour.h>
#include <ardour/playlist.h>

namespace ARDOUR  {

class Session;
class Region;
class AudioRegion;
class Source;

class AudioPlaylist : public ARDOUR::Playlist
{
  public:
	typedef std::list<Crossfade*> Crossfades;
	
   public:
	AudioPlaylist (Session&, const XMLNode&, bool hidden = false);
	AudioPlaylist (Session&, string name, bool hidden = false);
	AudioPlaylist (boost::shared_ptr<const AudioPlaylist>, string name, bool hidden = false);
	AudioPlaylist (boost::shared_ptr<const AudioPlaylist>, nframes_t start, nframes_t cnt, string name, bool hidden = false);

       ~AudioPlaylist (); /* public should use unref() */

	void clear (bool with_signals=true);

        nframes_t read (Sample *dst, Sample *mixdown, float *gain_buffer, nframes_t start, nframes_t cnt, uint32_t chan_n=0);

	int set_state (const XMLNode&);

	sigc::signal<void,Crossfade *> NewCrossfade; 

	template<class T> void foreach_crossfade (T *t, void (T::*func)(Crossfade *));
	void crossfades_at (nframes_t frame, Crossfades&);

	bool destroy_region (boost::shared_ptr<Region>);

    protected:

	/* playlist "callbacks" */
	void notify_crossfade_added (Crossfade *);
	void flush_notifications ();

	void finalize_split_region (boost::shared_ptr<Region> orig, boost::shared_ptr<Region> left, boost::shared_ptr<Region> right);
	
        void refresh_dependents (boost::shared_ptr<Region> region);
        void check_dependents (boost::shared_ptr<Region> region, bool norefresh);
        void remove_dependents (boost::shared_ptr<Region> region);

    private:
       Crossfades      _crossfades;    /* xfades currently in use */
       Crossfades      _pending_xfade_adds;

       void crossfade_invalidated (Crossfade*);
       XMLNode& state (bool full_state);
       void dump () const;

       bool region_changed (Change, boost::shared_ptr<Region>);
       void crossfade_changed (Change);
       void add_crossfade (Crossfade&);

       void source_offset_changed (boost::shared_ptr<AudioRegion> region);
};

} /* namespace ARDOUR */

#endif	/* __ardour_audio_playlist_h__ */


