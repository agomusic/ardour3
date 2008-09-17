/*
    Copyright (C) 2008 Paul Davis
    Author: Sakari Bergen

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

#ifndef __ardour_export_file_io_h__
#define __ardour_export_file_io_h__

#include <sndfile.h>

#include <ardour/graph.h>
#include <ardour/types.h>
#include <ardour/ardour.h>

namespace ARDOUR
{

/// Common part for all export file writers
class ExportFileWriter
{
  public:
	ExportFileWriter (string filename) : _filename (filename) {}
	virtual ~ExportFileWriter () {}
	
	string filename () const { return _filename; }
	nframes_t position () const { return _position; }
	
	void set_position (nframes_t position) { _position = position; }
	
  protected:
	string _filename;
	nframes_t _position;
};

/// Common interface for templated libsndfile writers
class SndfileWriterBase : public ExportFileWriter
{
  public:
	SndfileWriterBase (int channels, nframes_t samplerate, int format, string const & path);
	virtual ~SndfileWriterBase ();

	SNDFILE * get_sndfile () const { return sndfile; }

  protected:
	SF_INFO        sf_info;
	SNDFILE *      sndfile;
};


/// Template parameter specific parts of sndfile writer
template <typename T>
class SndfileWriter : public SndfileWriterBase, public GraphSink<T>
{
  public:
	SndfileWriter (int channels, nframes_t samplerate, int format, string const & path);
	virtual ~SndfileWriter () {}
	
	nframes_t write (T * data, nframes_t frames);
	
  protected:
	sf_count_t (*write_func)(SNDFILE *, const T *, sf_count_t);

  private:
	void init (); // Inits write function
};

/// Writes and reads a RAW tempfile (file aquired with tmpfile())
class ExportTempFile : public SndfileWriter<float>, public GraphSource<float>
{
  public:
	ExportTempFile (uint32_t channels, nframes_t samplerate);
	~ExportTempFile () {}
	
	/// Causes the file to be read from the beginning again
	void reset_read () { reading = false; }
	nframes_t read (float * data, nframes_t frames);
	
	/* Silence management */
	
	nframes_t trim_beginning (bool yn = true);
	nframes_t trim_end (bool yn = true);
	
	void set_silence_beginning (nframes_t frames);
	void set_silence_end (nframes_t frames);

  private:
	/* File access */
	
	sf_count_t get_length ();
	sf_count_t get_position ();
	sf_count_t get_read_position (); // get position seems to default to the write pointer
	sf_count_t locate_to (nframes_t frames);
	sf_count_t _read (float * data, nframes_t frames);
	
	uint32_t channels;
	bool reading;
	
	/* Silence related */
	
	/* start and end are used by read() */
	
	nframes_t start;
	nframes_t end;
	
	/* these are the silence processing results and state */
	
	void process_beginning ();
	void process_end ();
	
	bool beginning_processed;
	bool end_processed;
	
	nframes_t silent_frames_beginning;
	nframes_t silent_frames_end;
	
	/* Silence to add to start and end */
	
	nframes_t silence_beginning;
	nframes_t silence_end;
	
	/* Takes care that the end postion gets set at some stage */
	
	bool end_set;
	
};

} // namespace ARDOUR

#endif /* __ardour_export_file_io_h__ */