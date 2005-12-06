/*
    Copyright (C) 20002 Paul Davis 

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

#include <list>
#include <cerrno>

#include <ardour/ardour.h>
#include <ardour/session.h>
#include <ardour/tempo.h>
#include <ardour/io.h>

#include <sndfile.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;

Pool Session::Click::pool ("click", sizeof (Click), 128);

void
Session::click (jack_nframes_t start, jack_nframes_t nframes, jack_nframes_t offset)
{
	TempoMap::BBTPointList *points;
	jack_nframes_t end;
	Sample *buf;
	vector<Sample*> bufs;

	if (_click_io == 0) {
		return;
	}
	
	if (_transport_speed != 1.0 || !_clicking || click_data == 0) {
		_click_io->silence (nframes, offset);
		return;
	} 

	end = start + nframes;

	buf = _passthru_buffers[0];
	points = _tempo_map->get_points (start, end);

	if (points == 0) {
		goto run_clicks;
	}

	if (points->empty()) {
		delete points;
		goto run_clicks;
	}

	for (TempoMap::BBTPointList::iterator i = points->begin(); i != points->end(); ++i) {
		switch ((*i).type) {
		case TempoMap::Beat:
			if (click_emphasis_data == 0 || (click_emphasis_data && (*i).beat != 1)) {
				clicks.push_back (new Click ((*i).frame, click_length, click_data));
			}
			break;

		case TempoMap::Bar:
			if (click_emphasis_data) {
				clicks.push_back (new Click ((*i).frame, click_emphasis_length, click_emphasis_data));
			} 
			break;
		}
	}

  run_clicks:
	memset (buf, 0, sizeof (Sample) * nframes);

	for (list<Click*>::iterator i = clicks.begin(); i != clicks.end(); ) {

		jack_nframes_t copy;
		jack_nframes_t internal_offset;
		Click *clk;
		list<Click*>::iterator next;

		clk = *i;
		next = i;
		++next;
	
		if (clk->start < start) {
			internal_offset = 0;
		} else {
			internal_offset = clk->start - start;
		}

		if (nframes < internal_offset) {
		         /* we've just located or something.. 
			    effectively going backwards.
			    lets get the flock out of here */
		        break;
		}

		copy = min (clk->duration - clk->offset, nframes - internal_offset);

		memcpy (buf + internal_offset, &clk->data[clk->offset], copy * sizeof (Sample));

		clk->offset += copy;

		if (clk->offset >= clk->duration) {
			delete clk;
			clicks.erase (i);
		}


		i = next;
	}

	_click_io->deliver_output (_passthru_buffers, 1, nframes, offset);
}

void
Session::setup_click_sounds (int which)
{
	SNDFILE *sndfile;
	SF_INFO info;

	clear_clicks();

	if ((which == 0 || which == 1)) {
		
		if (click_data && click_data != default_click) {
			delete [] click_data;
			click_data = 0;
		}

		if (click_sound.length() == 0) {

			click_data = const_cast<Sample*> (default_click);
			click_length = default_click_length;

		} else {

			if ((sndfile = sf_open (click_sound.c_str(), SFM_READ, &info)) == 0) {
				char errbuf[256];
				sf_error_str (0, errbuf, sizeof (errbuf) - 1);
				warning << string_compose (_("cannot open click soundfile %1 (%2)"), click_sound, errbuf) << endmsg;
				_clicking = false;
				return;
			}
			
			click_data = new Sample[info.frames];
			click_length = info.frames;
			
			if (sf_read_float (sndfile, click_data, info.frames) != info.frames) {
				warning << _("cannot read data from click soundfile") << endmsg;			
				delete click_data;
				click_data = 0;
				_clicking = false;
			}
			
			sf_close (sndfile);

		}
	}
		
	if ((which == 0 || which == -1)) {

		if (click_emphasis_data && click_emphasis_data != default_click_emphasis) {
			delete [] click_emphasis_data;
			click_emphasis_data = 0;
		}

		if (click_emphasis_sound.length() == 0) {
			click_emphasis_data = const_cast<Sample*> (default_click_emphasis);
			click_emphasis_length = default_click_emphasis_length;
		} else {
			if ((sndfile = sf_open (click_emphasis_sound.c_str(), SFM_READ, &info)) == 0) {
				char errbuf[256];
				sf_error_str (0, errbuf, sizeof (errbuf) - 1);
				warning << string_compose (_("cannot open click emphasis soundfile %1 (%2)"), click_emphasis_sound, errbuf) << endmsg;
				return;
			}
			
			click_emphasis_data = new Sample[info.frames];
			click_emphasis_length = info.frames;
			
			if (sf_read_float (sndfile, click_emphasis_data, info.frames) != info.frames) {
				warning << _("cannot read data from click emphasis soundfile") << endmsg;			
				delete click_emphasis_data;
				click_emphasis_data = 0;
			}
			
			sf_close (sndfile);
		}
	}
}		

void
Session::clear_clicks ()
{
	LockMonitor lm (route_lock, __LINE__, __FILE__);

	for (Clicks::iterator i = clicks.begin(); i != clicks.end(); ++i) {
		delete *i;
	}

	clicks.clear ();
}

void
Session::set_click_sound (string path)
{
	if (path != click_sound) {
		click_sound = path;
		setup_click_sounds (1);
	}
}

void
Session::set_click_emphasis_sound (string path)
{
	if (path != click_emphasis_sound) {
		click_emphasis_sound = path;
		setup_click_sounds (-1);
	}
}

void
Session::set_clicking (bool yn)
{
	if (click_requested != yn) {
		click_requested = yn;
		
		if (yn) {
			if (_click_io && click_data) {
				_clicking = true;
			}
		} else {
			_clicking = false;
		}

		 ControlChanged (Clicking); /* EMIT SIGNAL */
	}
}

bool
Session::get_clicking () const
{
	return click_requested;
}

