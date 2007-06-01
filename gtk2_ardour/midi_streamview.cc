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

#include <cmath>
#include <cassert>
#include <utility>

#include <gtkmm.h>

#include <gtkmm2ext/gtk_ui.h>

#include <ardour/midi_playlist.h>
#include <ardour/midi_region.h>
#include <ardour/midi_source.h>
#include <ardour/midi_diskstream.h>
#include <ardour/midi_track.h>
#include <ardour/smf_source.h>
#include <ardour/region_factory.h>

#include "midi_streamview.h"
#include "region_view.h"
#include "midi_region_view.h"
#include "midi_time_axis.h"
#include "canvas-simplerect.h"
#include "region_selection.h"
#include "selection.h"
#include "public_editor.h"
#include "ardour_ui.h"
#include "rgb_macros.h"
#include "gui_thread.h"
#include "utils.h"
#include "color.h"
#include "simplerect.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Editing;

MidiStreamView::MidiStreamView (MidiTimeAxisView& tv)
	: StreamView (tv)
{
	if (tv.is_track())
		stream_base_color = color_map[cMidiTrackBase];
	else
		stream_base_color = color_map[cMidiBusBase];
	
	canvas_rect->property_fill_color_rgba() = stream_base_color;
	canvas_rect->property_outline_color_rgba() = color_map[cAudioTrackOutline];

	//use_rec_regions = tv.editor.show_waveforms_recording ();
	use_rec_regions = true;
}

MidiStreamView::~MidiStreamView ()
{
}


RegionView*
MidiStreamView::add_region_view_internal (boost::shared_ptr<Region> r, bool wait_for_waves)
{
	boost::shared_ptr<MidiRegion> region = boost::dynamic_pointer_cast<MidiRegion> (r);

	if (region == 0) {
		return NULL;
	}

	MidiRegionView *region_view;
	list<RegionView *>::iterator i;

	for (i = region_views.begin(); i != region_views.end(); ++i) {
		if ((*i)->region() == r) {
			
			/* great. we already have a MidiRegionView for this Region. use it again. */

			(*i)->set_valid (true);
			return NULL;
		}
	}
	
	// can't we all just get along?
	assert(_trackview.midi_track()->mode() != Destructive);

	region_view = new MidiRegionView (canvas_group, _trackview, region, 
			_samples_per_unit, region_color);

	region_view->init (region_color, wait_for_waves);
	region_views.push_front (region_view);
	
	/* follow global waveform setting */

	// FIXME
	//region_view->set_waveform_visible(_trackview.editor.show_waveforms());

	/* display events */
	region_view->begin_write();
	for (size_t i=0; i < region->midi_source(0)->model().n_events(); ++i)
		region_view->add_event(region->midi_source(0)->model().event_at(i));
	region_view->end_write();

	/* catch regionview going away */
	region->GoingAway.connect (bind (mem_fun (*this, &MidiStreamView::remove_region_view), region));
	
	RegionViewAdded (region_view);

	return region_view;
}

// FIXME: code duplication with AudioStreamVIew
void
MidiStreamView::redisplay_diskstream ()
{
	list<RegionView *>::iterator i, tmp;

	for (i = region_views.begin(); i != region_views.end(); ++i) {
		(*i)->set_valid (false);
	}

	if (_trackview.is_midi_track()) {
		_trackview.get_diskstream()->playlist()->foreach_region (static_cast<StreamView*>(this), &StreamView::add_region_view);
	}

	for (i = region_views.begin(); i != region_views.end(); ) {
		tmp = i;
		tmp++;

		if (!(*i)->is_valid()) {
			delete *i;
			region_views.erase (i);
		} 

		i = tmp;
	}

	/* now fix layering */

	for (RegionViewList::iterator i = region_views.begin(); i != region_views.end(); ++i) {
		region_layered (*i);
	}
}


void
MidiStreamView::setup_rec_box ()
{
	// cerr << _trackview.name() << " streamview SRB\n";

	if (_trackview.session().transport_rolling()) {

		if (!rec_active && 
		    _trackview.session().record_status() == Session::Recording && 
		    _trackview.get_diskstream()->record_enabled()) {

			if (use_rec_regions && rec_regions.size() == rec_rects.size()) {

				/* add a new region, but don't bother if they set use_rec_regions mid-record */

				MidiRegion::SourceList sources;
				
				for (list<sigc::connection>::iterator prc = rec_data_ready_connections.begin(); prc != rec_data_ready_connections.end(); ++prc) {
					(*prc).disconnect();
				}
				rec_data_ready_connections.clear();

				// FIXME
				boost::shared_ptr<MidiDiskstream> mds = boost::dynamic_pointer_cast<MidiDiskstream>(_trackview.get_diskstream());
				assert(mds);

				sources.push_back(mds->write_source());
				
				rec_data_ready_connections.push_back (mds->write_source()->ViewDataRangeReady.connect (bind (mem_fun (*this, &MidiStreamView::rec_data_range_ready), boost::weak_ptr<Source>(mds->write_source())))); 

				// handle multi
				
				jack_nframes_t start = 0;
				if (rec_regions.size() > 0) {
					start = rec_regions.back().first->start() + _trackview.get_diskstream()->get_captured_frames(rec_regions.size()-1);
				}
				
				boost::shared_ptr<MidiRegion> region (boost::dynamic_pointer_cast<MidiRegion>
					(RegionFactory::create (sources, start, 1 , "", 0, (Region::Flag)(Region::DefaultFlags | Region::DoNotSaveState), false)));
				assert(region);
				region->set_position (_trackview.session().transport_frame(), this);
				rec_regions.push_back (make_pair(region, (RegionView*)0));
				
				// rec regions are destroyed in setup_rec_box

				/* we add the region later */
			}
			
			/* start a new rec box */

			MidiTrack* mt = _trackview.midi_track(); /* we know what it is already */
			boost::shared_ptr<MidiDiskstream> ds = mt->midi_diskstream();
			jack_nframes_t frame_pos = ds->current_capture_start ();
			gdouble xstart = _trackview.editor.frame_to_pixel (frame_pos);
			gdouble xend;
			uint32_t fill_color;

			assert(_trackview.midi_track()->mode() == Normal);
			
			xend = xstart;
			fill_color = color_map[cRecordingRectFill];
			
			ArdourCanvas::SimpleRect * rec_rect = new Gnome::Canvas::SimpleRect (*canvas_group);
			rec_rect->property_x1() = xstart;
			rec_rect->property_y1() = 1.0;
			rec_rect->property_x2() = xend;
			rec_rect->property_y2() = (double) _trackview.height - 1;
			rec_rect->property_outline_color_rgba() = color_map[cRecordingRectOutline];
			rec_rect->property_fill_color_rgba() = fill_color;
			rec_rect->lower_to_bottom();
			
			RecBoxInfo recbox;
			recbox.rectangle = rec_rect;
			recbox.start = _trackview.session().transport_frame();
			recbox.length = 0;
			
			rec_rects.push_back (recbox);
			
			screen_update_connection.disconnect();
			screen_update_connection = ARDOUR_UI::instance()->SuperRapidScreenUpdate.connect (mem_fun (*this, &MidiStreamView::update_rec_box));	
			rec_updating = true;
			rec_active = true;

		} else if (rec_active &&
			   (_trackview.session().record_status() != Session::Recording ||
			    !_trackview.get_diskstream()->record_enabled())) {

			screen_update_connection.disconnect();
			rec_active = false;
			rec_updating = false;

		}
		
	} else {

		// cerr << "\tNOT rolling, rec_rects = " << rec_rects.size() << " rec_regions = " << rec_regions.size() << endl;

		if (!rec_rects.empty() || !rec_regions.empty()) {

			/* disconnect rapid update */
			screen_update_connection.disconnect();

			for (list<sigc::connection>::iterator prc = rec_data_ready_connections.begin(); prc != rec_data_ready_connections.end(); ++prc) {
				(*prc).disconnect();
			}
			rec_data_ready_connections.clear();

			rec_updating = false;
			rec_active = false;
			
			/* remove temp regions */
			
			for (list<pair<boost::shared_ptr<Region>,RegionView*> >::iterator iter = rec_regions.begin(); iter != rec_regions.end();) {
				list<pair<boost::shared_ptr<Region>,RegionView*> >::iterator tmp;
				
				tmp = iter;
				++tmp;

				(*iter).first->drop_references ();

				iter = tmp;
			}
			
			rec_regions.clear();

			// cerr << "\tclear " << rec_rects.size() << " rec rects\n";

			/* transport stopped, clear boxes */
			for (vector<RecBoxInfo>::iterator iter=rec_rects.begin(); iter != rec_rects.end(); ++iter) {
				RecBoxInfo &rect = (*iter);
				delete rect.rectangle;
			}
			
			rec_rects.clear();
			
		}
	}
}

void
MidiStreamView::update_rec_regions (boost::shared_ptr<MidiBuffer> data, nframes_t start, nframes_t dur)
{
	ENSURE_GUI_THREAD (bind (mem_fun (*this, &MidiStreamView::update_rec_regions), data, start, dur));

	if (use_rec_regions) {

		uint32_t n = 0;

		for (list<pair<boost::shared_ptr<Region>,RegionView*> >::iterator iter = rec_regions.begin(); iter != rec_regions.end(); n++) {

			list<pair<boost::shared_ptr<Region>,RegionView*> >::iterator tmp;

			tmp = iter;
			++tmp;

			if (!canvas_item_visible (rec_rects[n].rectangle)) {
				/* rect already hidden, this region is done */
				iter = tmp;
				continue;
			}
			
			boost::shared_ptr<MidiRegion> region = boost::dynamic_pointer_cast<MidiRegion>(iter->first);
			if (!region) {
				continue;
			}

			nframes_t origlen = region->length();
			
			//cerr << "MIDI URR: " << start << " * " << dur
			//	<< " (origlen " << origlen << ")" << endl;

			if (region == rec_regions.back().first && rec_active) {

				if (start >= region->start()) {

					nframes_t nlen = start + dur - region->start();

					if (nlen != region->length()) {

						region->freeze ();
						region->set_position (_trackview.get_diskstream()->get_capture_start_frame(n), this);
						region->set_length (nlen, this);
						region->thaw ("updated");

						if (origlen == 1) {
							/* our special initial length */
							iter->second = add_region_view_internal (region, false);
							((MidiRegionView*)iter->second)->begin_write();
						}

						/* also update rect */
						ArdourCanvas::SimpleRect * rect = rec_rects[n].rectangle;
						gdouble xend = _trackview.editor.frame_to_pixel (region->position() + region->length());
						rect->property_x2() = xend;

						/* draw events */
						MidiRegionView* mrv = (MidiRegionView*)iter->second;
						for (size_t i = 0; i < data->size(); ++i) {
							const MidiEvent& ev = (*data.get())[i];
							mrv->add_event(ev);
						}

					}
				}

			} else {

				nframes_t nlen = _trackview.get_diskstream()->get_captured_frames(n);

				if (nlen != region->length()) {

					if (region->source(0)->length() >= region->start() + nlen) {

						region->freeze ();
						region->set_position (_trackview.get_diskstream()->get_capture_start_frame(n), this);
						region->set_length (nlen, this);
						region->thaw ("updated");
						
						if (origlen == 1) {
							/* our special initial length */
							iter->second = add_region_view_internal (region, false);
						}
						
						/* also hide rect */
						ArdourCanvas::Item * rect = rec_rects[n].rectangle;
						rect->hide();

					}
				}
			}

			iter = tmp;
		}
	}
}

void
MidiStreamView::rec_data_range_ready (boost::shared_ptr<MidiBuffer> data, jack_nframes_t start, jack_nframes_t dur, boost::weak_ptr<Source> weak_src)
{
	// this is called from the butler thread for now
	
	ENSURE_GUI_THREAD(bind (mem_fun (*this, &MidiStreamView::rec_data_range_ready), data, start, dur, weak_src));
	
	boost::shared_ptr<SMFSource> src (boost::dynamic_pointer_cast<SMFSource>(weak_src.lock()));
	
	//cerr << src.get() << " MIDI READY: " << start << " * " << dur
	//	<< " -- " << data->size() << " events!" << endl;
	
	this->update_rec_regions (data, start, dur);
}

void
MidiStreamView::color_handler (ColorID id, uint32_t val)
{
	switch (id) {
	case cMidiTrackBase:
		if (_trackview.is_midi_track()) {
			canvas_rect->property_fill_color_rgba() = val;
		} 
		break;
	case cMidiBusBase:
		if (!_trackview.is_midi_track()) {
			canvas_rect->property_fill_color_rgba() = val;
		}
		break;
	case cMidiTrackOutline:
		canvas_rect->property_outline_color_rgba() = val;
		break;

	default:
		break;
	}
}
