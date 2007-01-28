/*
    Copyright (C) 2001-2006 Paul Davis 

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

#ifndef __gtk_ardour_audio_region_view_h__
#define __gtk_ardour_audio_region_view_h__

#include <vector>

#include <libgnomecanvasmm.h>
#include <libgnomecanvasmm/polygon.h>
#include <sigc++/signal.h>
#include <ardour/audioregion.h>

#include "region_view.h"
#include "route_time_axis.h"
#include "time_axis_view_item.h"
#include "automation_line.h"
#include "enums.h"
#include "waveview.h"
#include "canvas.h"
#include "color.h"

namespace ARDOUR {
	class AudioRegion;
	class PeakData;
};

class AudioTimeAxisView;
class AudioRegionGainLine;
class AudioRegionEditor;
class GhostRegion;
class AutomationTimeAxisView;

class AudioRegionView : public RegionView
{
  public:
	AudioRegionView (ArdourCanvas::Group *, 
			 RouteTimeAxisView&,
			 boost::shared_ptr<ARDOUR::AudioRegion>,
			 double initial_samples_per_unit,
			 Gdk::Color& basic_color);

	~AudioRegionView ();
	
	virtual void init (Gdk::Color& base_color, bool wait_for_data = false);
	
	boost::shared_ptr<ARDOUR::AudioRegion> audio_region() const;
	
	void set_height (double);
	void set_samples_per_unit (double);
	
	void set_amplitude_above_axis (gdouble spp);
	
	void temporarily_hide_envelope (); ///< Dangerous!
	void unhide_envelope ();           ///< Dangerous!
	
	void set_envelope_visible (bool);
	void set_waveform_visible (bool yn);
	void set_waveform_shape (WaveformShape);
	void set_waveform_scale (WaveformScale);
	
	bool waveform_rectified() const { return _flags & WaveformRectified; }
	bool waveform_logscaled() const { return _flags & WaveformLogScaled; }
	bool waveform_visible()   const { return _flags & WaveformVisible; }
	bool envelope_visible()   const { return _flags & EnvelopeVisible; }
	
	void show_region_editor ();
	
	void add_gain_point_event (ArdourCanvas::Item *item, GdkEvent *event);
	void remove_gain_point_event (ArdourCanvas::Item *item, GdkEvent *event);
	
	AudioRegionGainLine* get_gain_line() const { return gain_line; }
	
	void region_changed (ARDOUR::Change);
	void envelope_active_changed ();
	
	GhostRegion* add_ghost (AutomationTimeAxisView&);
	
	void reset_fade_in_shape_width (nframes_t);
	void reset_fade_out_shape_width (nframes_t);

	virtual void entered ();
	virtual void exited ();
	
  protected:

    /* this constructor allows derived types
       to specify their visibility requirements
       to the TimeAxisViewItem parent class
    */
    
    AudioRegionView (ArdourCanvas::Group *, 
		     RouteTimeAxisView&,
		     boost::shared_ptr<ARDOUR::AudioRegion>,
		     double      samples_per_unit,
		     Gdk::Color& basic_color,
		     TimeAxisViewItem::Visibility);
    
    enum Flags {
	    EnvelopeVisible = 0x1,
	    WaveformVisible = 0x4,
	    WaveformRectified = 0x8,
	    WaveformLogScaled = 0x10,
    };

    vector<ArdourCanvas::WaveView *> waves;
    vector<ArdourCanvas::WaveView *> tmp_waves; ///< see ::create_waves()
    ArdourCanvas::Polygon*           sync_mark; ///< polgyon for sync position 
    ArdourCanvas::SimpleLine*        zero_line;
    ArdourCanvas::Polygon*           fade_in_shape;
    ArdourCanvas::Polygon*           fade_out_shape;
    ArdourCanvas::SimpleRect*        fade_in_handle;
    ArdourCanvas::SimpleRect*        fade_out_handle;
    
    AudioRegionGainLine * gain_line;

    double _amplitude_above_axis;

    uint32_t _flags;
    uint32_t fade_color;
    
    void reset_fade_shapes ();
    void reset_fade_in_shape ();
    void reset_fade_out_shape ();
    void fade_in_changed ();
    void fade_out_changed ();
    void fade_in_active_changed ();
    void fade_out_active_changed ();

    void region_resized (ARDOUR::Change);
    void region_moved (void *);
    void region_muted ();
    void region_scale_amplitude_changed ();
	void region_renamed ();

    void create_waves ();
    void create_one_wave (uint32_t, bool);
    void manage_zero_line ();
    void peaks_ready_handler (uint32_t);
    void set_flags (XMLNode *);
    void store_flags ();

    void set_colors ();
    void compute_colors (Gdk::Color&);
    void reset_width_dependent_items (double pixel_width);
    void set_waveview_data_src();

    vector<GnomeCanvasWaveViewCache*> wave_caches;
    
    void color_handler (ColorID, uint32_t);
};

#endif /* __gtk_ardour_audio_region_view_h__ */
