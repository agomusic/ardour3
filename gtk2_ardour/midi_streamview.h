/*
    Copyright (C) 2001, 2006 Paul Davis 

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

#ifndef __ardour_midi_streamview_h__
#define __ardour_midi_streamview_h__

#include <list>
#include <cmath>

#include <ardour/location.h>
#include "enums.h"
#include "simplerect.h"
#include "color.h"
#include "streamview.h"

namespace Gdk {
	class Color;
}

namespace ARDOUR {
	class Route;
	class Diskstream;
	class Crossfade;
	class PeakData;
	class MidiRegion;
	class Source;
}

class PublicEditor;
class Selectable;
class MidiTimeAxisView;
class MidiRegionView;
class RegionSelection;
class CrossfadeView;
class Selection;

class MidiStreamView : public StreamView
{
  public:
	MidiStreamView (MidiTimeAxisView&);
	~MidiStreamView ();

	void set_selected_regionviews (RegionSelection&);
	void get_selectables (jack_nframes_t start, jack_nframes_t end, list<Selectable* >&);
	void get_inverted_selectables (Selection&, list<Selectable* >& results);

  private:
	void setup_rec_box ();
	void rec_data_range_ready (boost::shared_ptr<ARDOUR::MidiBuffer> data, jack_nframes_t start, jack_nframes_t dur, boost::weak_ptr<ARDOUR::Source> src); 
	void update_rec_regions (boost::shared_ptr<ARDOUR::MidiBuffer> data, jack_nframes_t start, jack_nframes_t dur);
	
	RegionView* add_region_view_internal (boost::shared_ptr<ARDOUR::Region>, bool wait_for_waves);

	void redisplay_diskstream ();

	void color_handler (ColorID id, uint32_t val);
};

#endif /* __ardour_midi_streamview_h__ */
