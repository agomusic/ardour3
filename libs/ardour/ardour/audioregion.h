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

*/

#ifndef __ardour_audio_region_h__
#define __ardour_audio_region_h__

#include <vector>

#include <pbd/fastlog.h>
#include <pbd/undo.h>

#include <ardour/ardour.h>
#include <ardour/region.h>
#include <ardour/gain.h>
#include <ardour/logcurve.h>
#include <ardour/export.h>

class XMLNode;

namespace ARDOUR {

class Route;
class Playlist;
class Session;
class Filter;
class AudioSource;

class AudioRegion : public Region
{
  public:
	static Change FadeInChanged;
	static Change FadeOutChanged;
	static Change FadeInActiveChanged;
	static Change FadeOutActiveChanged;
	static Change EnvelopeActiveChanged;
	static Change ScaleAmplitudeChanged;
	static Change EnvelopeChanged;

	~AudioRegion();

	bool speed_mismatch (float) const;

	boost::shared_ptr<AudioSource> audio_source (uint32_t n=0) const;

	void   set_scale_amplitude (gain_t);
	gain_t scale_amplitude() const { return _scale_amplitude; }
	
	void normalize_to (float target_in_dB = 0.0f);

	bool envelope_active () const { return _flags & Region::EnvelopeActive; }
	bool fade_in_active ()  const { return _flags & Region::FadeIn; }
	bool fade_out_active () const { return _flags & Region::FadeOut; }

	boost::shared_ptr<AutomationList> fade_in()  { return _fade_in; }
	boost::shared_ptr<AutomationList> fade_out() { return _fade_out; }
	boost::shared_ptr<AutomationList> envelope() { return _envelope; }

	virtual nframes_t read_peaks (PeakData *buf, nframes_t npeaks,
				      nframes_t offset, nframes_t cnt,
				      uint32_t chan_n=0, double samples_per_unit= 1.0) const;
	
	virtual nframes_t read_at (Sample *buf, Sample *mixdown_buf,
				   float *gain_buf, nframes_t position, nframes_t cnt, 
				   uint32_t       chan_n      = 0) const;
	
	virtual nframes_t master_read_at (Sample *buf, Sample *mixdown_buf, 
					  float *gain_buf,
					  nframes_t position, nframes_t cnt, uint32_t chan_n=0) const;
	
	virtual nframes_t read_raw_internal (Sample*, nframes_t, nframes_t) const;

	XMLNode& state (bool);
	int      set_state (const XMLNode&);

	static void set_default_fade (float steepness, nframes_t len);
	bool fade_in_is_default () const;
	bool fade_out_is_default () const;

	enum FadeShape {
		Linear,
		Fast,
		Slow,
		LogA,
		LogB
	};

	void set_fade_in_active (bool yn);
	void set_fade_in_shape (FadeShape);
	void set_fade_in_length (nframes_t);
	void set_fade_in (FadeShape, nframes_t);

	void set_fade_out_active (bool yn);
	void set_fade_out_shape (FadeShape);
	void set_fade_out_length (nframes_t);
	void set_fade_out (FadeShape, nframes_t);

	void set_envelope_active (bool yn);
	void set_default_envelope ();

	int separate_by_channel (ARDOUR::Session&, vector<boost::shared_ptr<AudioRegion> >&) const;

	/* export */

	int exportme (ARDOUR::Session&, ARDOUR::AudioExportSpecification&);

	/* xfade/fade interactions */

	void suspend_fade_in ();
	void suspend_fade_out ();
	void resume_fade_in ();
	void resume_fade_out ();

  private:
	friend class RegionFactory;

	AudioRegion (boost::shared_ptr<AudioSource>, nframes_t start, nframes_t length);
	AudioRegion (boost::shared_ptr<AudioSource>, nframes_t start, nframes_t length, const string& name, layer_t = 0, Region::Flag flags = Region::DefaultFlags);
	AudioRegion (SourceList &, nframes_t start, nframes_t length, const string& name, layer_t = 0, Region::Flag flags = Region::DefaultFlags);
	AudioRegion (boost::shared_ptr<const AudioRegion>, nframes_t start, nframes_t length, const string& name, layer_t = 0, Region::Flag flags = Region::DefaultFlags);
	AudioRegion (boost::shared_ptr<AudioSource>, const XMLNode&);
	AudioRegion (SourceList &, const XMLNode&);

  private:
	void init ();
	void set_default_fades ();
	void set_default_fade_in ();
	void set_default_fade_out ();

	void recompute_gain_at_end ();
	void recompute_gain_at_start ();

	nframes_t _read_at (const SourceList&, Sample *buf, Sample *mixdown_buffer, 
			    float *gain_buffer, nframes_t position, nframes_t cnt, 
			    uint32_t chan_n = 0) const;

	void recompute_at_start ();
	void recompute_at_end ();

	void envelope_changed ();
	void fade_in_changed ();
	void fade_out_changed ();
	void source_offset_changed ();
	void listen_to_my_curves ();

	boost::shared_ptr<AutomationList> _fade_in;
	FadeShape                         _fade_in_shape;
	boost::shared_ptr<AutomationList> _fade_out;
	FadeShape                         _fade_out_shape;
	boost::shared_ptr<AutomationList> _envelope;
	gain_t                            _scale_amplitude;
	uint32_t                          _fade_in_disabled;
	uint32_t                          _fade_out_disabled;

  protected:
	/* default constructor for derived (compound) types */

	AudioRegion (nframes_t, nframes_t, std::string name); 
	AudioRegion (boost::shared_ptr<const AudioRegion>);

	int set_live_state (const XMLNode&, Change&, bool send);
	
	virtual bool verify_start (nframes_t);
	virtual bool verify_start_and_length (nframes_t, nframes_t);
	virtual bool verify_start_mutable (nframes_t&_start);
	virtual bool verify_length (nframes_t);
	/*virtual void recompute_at_start () = 0;
	virtual void recompute_at_end () = 0;*/
};

} /* namespace ARDOUR */

/* access from C objects */

extern "C" {
	int    region_read_peaks_from_c   (void *arg, uint32_t npeaks, uint32_t start, uint32_t length, intptr_t data, uint32_t n_chan, double samples_per_unit);
	uint32_t region_length_from_c (void *arg);
	uint32_t sourcefile_length_from_c (void *arg, double);
}

#endif /* __ardour_audio_region_h__ */
