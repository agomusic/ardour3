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

#ifndef __ardour_audio_source_h__
#define __ardour_audio_source_h__

#include <list>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <time.h>

#include <glibmm/thread.h>
#include <glibmm/ustring.h>

#include <sigc++/signal.h>

#include <ardour/source.h>
#include <ardour/ardour.h>
#include <pbd/stateful.h> 
#include <pbd/xml++.h>

using std::list;
using std::vector;

namespace ARDOUR {

class AudioSource : public Source, public boost::enable_shared_from_this<ARDOUR::AudioSource>
{
  public:
	AudioSource (Session&, Glib::ustring name);
	AudioSource (Session&, const XMLNode&);
	virtual ~AudioSource ();

	nframes64_t readable_length() const { return _length; }
	uint32_t    n_channels() const { return 1; }

	virtual nframes_t available_peaks (double zoom) const;

	/* stopgap until nframes_t becomes nframes64_t. this function is needed by the Readable interface */

	virtual nframes64_t read (Sample *dst, nframes64_t start, nframes64_t cnt, int channel) const {
		/* XXX currently ignores channel, assuming that source is always mono, which
		   historically has been true.
		*/
		return read (dst, (nframes_t) start, (nframes_t) cnt);
	}

	virtual nframes_t read (Sample *dst, nframes_t start, nframes_t cnt) const;
	virtual nframes_t write (Sample *src, nframes_t cnt);

	virtual float sample_rate () const = 0;

	virtual void mark_for_remove() = 0;
	virtual void mark_streaming_write_completed () {}

	virtual bool can_truncate_peaks() const { return true; }

	void set_captured_for (Glib::ustring str) { _captured_for = str; }
	Glib::ustring captured_for() const { return _captured_for; }

	uint32_t read_data_count() const { return _read_data_count; }
	uint32_t write_data_count() const { return _write_data_count; }

 	int read_peaks (PeakData *peaks, nframes_t npeaks, nframes_t start, nframes_t cnt, double samples_per_visual_peak) const;

 	int  build_peaks ();
	bool peaks_ready (sigc::slot<void>, sigc::connection&) const;

	mutable sigc::signal<void>  PeaksReady;
	mutable sigc::signal<void,nframes_t,nframes_t>  PeakRangeReady;
	
	XMLNode& get_state ();
	int set_state (const XMLNode&);

	int rename_peakfile (Glib::ustring newpath);
	void touch_peakfile ();

	static void set_build_missing_peakfiles (bool yn) {
		_build_missing_peakfiles = yn;
	}

	static void set_build_peakfiles (bool yn) {
		_build_peakfiles = yn;
	}

	static bool get_build_peakfiles () {
		return _build_peakfiles;
	}

	virtual int setup_peakfile () { return 0; }

	int prepare_for_peakfile_writes ();
	void done_with_peakfile_writes (bool done = true);

	std::vector<nframes64_t> transients;
	std::string get_transients_path() const;

  protected:
	static bool _build_missing_peakfiles;
	static bool _build_peakfiles;

	bool                 _peaks_built;
	mutable Glib::Mutex  _lock;
	mutable Glib::Mutex  _peaks_ready_lock;
	Glib::ustring         peakpath;
	Glib::ustring        _captured_for;

	mutable uint32_t _read_data_count;  // modified in read()
	mutable uint32_t _write_data_count; // modified in write()

	int initialize_peakfile (bool newfile, Glib::ustring path);
	int build_peaks_from_scratch ();
	int compute_and_write_peaks (Sample* buf, nframes_t first_frame, nframes_t cnt, bool force, bool intermediate_peaks_ready_signal);
	void truncate_peakfile();

	mutable off_t _peak_byte_max; // modified in compute_and_write_peak()

	virtual nframes_t read_unlocked (Sample *dst, nframes_t start, nframes_t cnt) const = 0;
	virtual nframes_t write_unlocked (Sample *dst, nframes_t cnt) = 0;
	virtual Glib::ustring peak_path(Glib::ustring audio_path) = 0;
	virtual Glib::ustring find_broken_peakfile (Glib::ustring missing_peak_path, Glib::ustring audio_path) = 0;
	
	void update_length (nframes_t pos, nframes_t cnt);

 	virtual int read_peaks_with_fpp (PeakData *peaks, nframes_t npeaks, nframes_t start, nframes_t cnt, 
					 double samples_per_visual_peak, nframes_t fpp) const;

	int compute_and_write_peaks (Sample* buf, nframes_t first_frame, nframes_t cnt, bool force, 
				     bool intermediate_peaks_ready_signal, nframes_t frames_per_peak);

	int load_transients (const std::string&);

  private:
	int peakfile;
	nframes_t peak_leftover_cnt;
	nframes_t peak_leftover_size;
	Sample* peak_leftovers;
	nframes_t peak_leftover_frame;

	bool file_changed (Glib::ustring path);
};

}

#endif /* __ardour_audio_source_h__ */
