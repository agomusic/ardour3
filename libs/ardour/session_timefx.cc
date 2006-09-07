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

#include <cerrno>
#include <stdexcept>

#include <pbd/basename.h>

#include <soundtouch/SoundTouch.h>

#include <ardour/session.h>
#include <ardour/audioregion.h>
#include <ardour/sndfilesource.h>
#include <ardour/region_factory.h>
#include <ardour/source_factory.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace soundtouch;

boost::shared_ptr<AudioRegion>
Session::tempoize_region (TimeStretchRequest& tsr)
{
	SourceList sources;
	SourceList::iterator it;
	boost::shared_ptr<AudioRegion> r;
	SoundTouch st;
	string region_name;
	string ident = X_("-TIMEFX-");
	float percentage;
	jack_nframes_t total_frames;
	jack_nframes_t done;

	/* the soundtouch code wants a *tempo* change percentage, which is 
	   of opposite sign to the length change.  
	*/

	percentage = -tsr.fraction;

	st.setSampleRate (frame_rate());
	st.setChannels (1);
	st.setTempoChange (percentage);
	st.setPitchSemiTones (0);
	st.setRateChange (0);
	
	st.setSetting(SETTING_USE_QUICKSEEK, tsr.quick_seek);
	st.setSetting(SETTING_USE_AA_FILTER, tsr.antialias);

	vector<string> names = tsr.region->master_source_names();

	tsr.progress = 0.0f;
	total_frames = tsr.region->length() * tsr.region->n_channels();
	done = 0;

	for (uint32_t i = 0; i < tsr.region->n_channels(); ++i) {
		string path = path_from_region_name (PBD::basename_nosuffix (names[i]), ident);

		if (path.length() == 0) {
			error << string_compose (_("tempoize: error creating name for new audio file based on %1"), tsr.region->name()) 
			      << endmsg;
			goto out;
		}

		try {
			sources.push_back (boost::dynamic_pointer_cast<AudioFileSource> (SourceFactory::createWritable (*this, path, false, frame_rate())));

		} catch (failed_constructor& err) {
			error << string_compose (_("tempoize: error creating new audio file %1 (%2)"), path, strerror (errno)) << endmsg;
			goto out;
		}
	}
	
	try {
		const jack_nframes_t bufsize = 16384;

		for (uint32_t i = 0; i < sources.size(); ++i) {
			gain_t gain_buffer[bufsize];
			Sample buffer[bufsize];
			jack_nframes_t pos = 0;
			jack_nframes_t this_read = 0;

			st.clear();
			while (tsr.running && pos < tsr.region->length()) {
				jack_nframes_t this_time;
			
				this_time = min (bufsize, tsr.region->length() - pos);

				/* read from the master (original) sources for the region, 
				   not the ones currently in use, in case it's already been 
				   subject to timefx.  */

				if ((this_read = tsr.region->master_read_at (buffer, buffer, gain_buffer, pos + tsr.region->position(), this_time)) != this_time) {
					error << string_compose (_("tempoize: error reading data from %1"), sources[i]->name()) << endmsg;
					goto out;
				}
			
				pos += this_read;
				done += this_read;

				tsr.progress = (float) done / total_frames;
				
				st.putSamples (buffer, this_read);
			
				while ((this_read = st.receiveSamples (buffer, bufsize)) > 0 && tsr.running) {
					if (sources[i]->write (buffer, this_read) != this_read) {
						error << string_compose (_("error writing tempo-adjusted data to %1"), sources[i]->name()) << endmsg;
						goto out;
					}
				}
			}
		
			if (tsr.running) {
				st.flush ();
			}
		
			while (tsr.running && (this_read = st.receiveSamples (buffer, bufsize)) > 0) {
				if (sources[i]->write (buffer, this_read) != this_read) {
					error << string_compose (_("error writing tempo-adjusted data to %1"), sources[i]->name()) << endmsg;
					goto out;
				}
			}
		}
	} catch (runtime_error& err) {
		error << _("timefx code failure. please notify ardour-developers.") << endmsg;
		error << err.what() << endmsg;
		goto out;
	}

	time_t now;
	struct tm* xnow;
	time (&now);
	xnow = localtime (&now);

	for (it = sources.begin(); it != sources.end(); ++it) {
		boost::shared_ptr<AudioFileSource> afs = boost::dynamic_pointer_cast<AudioFileSource>(*it);
		if (afs) {
			afs->update_header (tsr.region->position(), *xnow, now);
		}
	}

	region_name = tsr.region->name() + X_(".t");

	r = (boost::dynamic_pointer_cast<AudioRegion> (RegionFactory::create (sources, 0, sources.front()->length(), region_name,
									      0, AudioRegion::Flag (AudioRegion::DefaultFlags | AudioRegion::WholeFile))));
	     
  out:

	if (sources.size()) {

		/* if we failed to complete for any reason, mark the new file 
		   for deletion.
		*/

		if ((!r || !tsr.running)) {
			for (it = sources.begin(); it != sources.end(); ++it) {
				(*it)->mark_for_remove ();
			}
		}

		sources.clear ();
	}
	
	/* if the process was cancelled, delete the region */

	if (!tsr.running) {
		r.reset ();
	}
	
	return r;
}
