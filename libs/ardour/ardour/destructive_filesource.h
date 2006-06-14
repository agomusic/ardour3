/*
    Copyright (C) 2006 Paul Davis 

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

#ifndef __ardour_destructive_file_source_h__ 
#define __ardour_destructive_file_source_h__

#include <string>

#include <ardour/sndfilesource.h>

struct tm;

namespace ARDOUR {

class DestructiveFileSource : public SndFileSource {
  public:
	DestructiveFileSource (std::string path, SampleFormat samp_format, HeaderFormat hdr_format, jack_nframes_t rate,
			       Flag flags = AudioFileSource::Flag (AudioFileSource::Writable|
								   AudioFileSource::BuildPeaks));

	DestructiveFileSource (const XMLNode&);
	~DestructiveFileSource ();

	jack_nframes_t last_capture_start_frame() const;
	void mark_capture_start (jack_nframes_t);
	void mark_capture_end ();
	void clear_capture_marks();

	XMLNode& get_state ();

	static void setup_standard_crossfades (jack_nframes_t sample_rate);

  protected:
	jack_nframes_t write_unlocked (Sample *src, jack_nframes_t start, jack_nframes_t cnt, char * workbuf);

  private:
	static jack_nframes_t xfade_frames;
	static gain_t* out_coefficient;
	static gain_t* in_coefficient;

	bool          _capture_start;
	bool          _capture_end;
	jack_nframes_t capture_start_frame;
	jack_nframes_t file_pos; // unit is frames
	Sample*        xfade_buf;

	jack_nframes_t crossfade (Sample* data, jack_nframes_t cnt, int dir, char * workbuf);
	void set_timeline_position (jack_nframes_t);
};

}

#endif /* __ardour_destructive_file_source_h__ */
