/*
    Copyright (C) 2008 Paul Davis 

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

#include <ardour/analyser.h>
#include <ardour/audiofilesource.h>
#include <ardour/transient_detector.h>

#include <pbd/pthread_utils.h>
#include <pbd/convert.h>

using namespace std;
using namespace sigc;
using namespace ARDOUR;
using namespace PBD;

Analyser* Analyser::the_analyser = 0;
Glib::StaticMutex Analyser::analysis_queue_lock = GLIBMM_STATIC_MUTEX_INIT;
Glib::Cond* Analyser::SourcesToAnalyse = 0;
list<boost::weak_ptr<Source> > Analyser::analysis_queue;

Analyser::Analyser ()
{
	
}

Analyser::~Analyser ()
{
}

static void
analyser_work ()
{
	Analyser::work ();
}

void
Analyser::init ()
{
	SourcesToAnalyse = new Glib::Cond();
	Glib::Thread::create (sigc::ptr_fun (analyser_work), false);
}

void
Analyser::queue_source_for_analysis (boost::shared_ptr<Source> src, bool force)
{
	if (!src->can_be_analysed()) {
		return;
	}

	if (!force && src->has_been_analysed()) {
		return;
	}

	Glib::Mutex::Lock lm (analysis_queue_lock);
	analysis_queue.push_back (boost::weak_ptr<Source>(src));
	SourcesToAnalyse->broadcast ();
}

void
Analyser::work ()
{
	PBD::ThreadCreated (pthread_self(), string ("analyser-") + to_string (pthread_self(), std::dec));

	while (true) {
		analysis_queue_lock.lock ();

	  wait:
		if (analysis_queue.empty()) {
			SourcesToAnalyse->wait (analysis_queue_lock);
		}

		if (analysis_queue.empty()) {
			goto wait;
		}

		boost::shared_ptr<Source> src (analysis_queue.front().lock());
		analysis_queue.pop_front();
		analysis_queue_lock.unlock ();

		boost::shared_ptr<AudioFileSource> afs = boost::dynamic_pointer_cast<AudioFileSource> (src);

		if (afs && afs->length()) {
			analyse_audio_file_source (afs);
		}
	}
}

void
Analyser::analyse_audio_file_source (boost::shared_ptr<AudioFileSource> src)
{
	AnalysisFeatureList results;

	TransientDetector td (src->sample_rate());

	if (td.run (src->get_transients_path(), src.get(), 0, results) == 0) {
		src->set_been_analysed (true);
	} else {
		src->set_been_analysed (false);
	}

}

		
