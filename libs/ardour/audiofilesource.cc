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

*/

#include <vector>

#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <pbd/mountpoint.h>
#include <pbd/pathscanner.h>
#include <pbd/stl_delete.h>
#include <pbd/strsplit.h>

#include <sndfile.h>

#include <glibmm/miscutils.h>

#include <ardour/audiofilesource.h>
#include <ardour/sndfile_helpers.h>
#include <ardour/sndfilesource.h>
#include <ardour/session.h>
#include <ardour/source_factory.h>

// if these headers come before sigc++ is included
// the parser throws ObjC++ errors. (nil is a keyword)
#ifdef HAVE_COREAUDIO 
#include <ardour/coreaudiosource.h>
#include <AudioToolbox/ExtendedAudioFile.h>
#include <AudioToolbox/AudioFormat.h>
#endif // HAVE_COREAUDIO

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;

string AudioFileSource::peak_dir = "";
string AudioFileSource::search_path;

sigc::signal<void> AudioFileSource::HeaderPositionOffsetChanged;
uint64_t           AudioFileSource::header_position_offset = 0;

/* XXX maybe this too */
char   AudioFileSource::bwf_serial_number[13] = "000000000000";

AudioFileSource::AudioFileSource (Session& s, string idstr, Flag flags)
	: AudioSource (s, idstr), _flags (flags)
{
	/* constructor used for existing external to session files. file must exist already */
	_is_embedded = AudioFileSource::determine_embeddedness (idstr);

	if (init (idstr, true)) {
		throw failed_constructor ();
	}

}

AudioFileSource::AudioFileSource (Session& s, std::string path, Flag flags, SampleFormat samp_format, HeaderFormat hdr_format)
	: AudioSource (s, path), _flags (flags)
{
	/* constructor used for new internal-to-session files. file cannot exist */
	_is_embedded = false;

	if (init (path, false)) {
		throw failed_constructor ();
	}
}

AudioFileSource::AudioFileSource (Session& s, const XMLNode& node)
	: AudioSource (s, node), _flags (Flag (Writable|CanRename))
{
	/* constructor used for existing internal-to-session files. file must exist */

	if (set_state (node)) {
		throw failed_constructor ();
	}
	
	if (init (_name, true)) {
		throw failed_constructor ();
	}
}

AudioFileSource::~AudioFileSource ()
{
	if (removable()) {
		unlink (_path.c_str());
		unlink (peakpath.c_str());
	}
}

bool
AudioFileSource::determine_embeddedness (std::string path)
{
	return (path.find("/") == 0);
}

bool
AudioFileSource::removable () const
{
	return (_flags & Removable) && ((_flags & RemoveAtDestroy) || ((_flags & RemovableIfEmpty) && length() == 0));
}

int
AudioFileSource::init (string pathstr, bool must_exist)
{
	bool is_new = false;

	_length = 0;
	timeline_position = 0;
	next_peak_clear_should_notify = false;
	_peaks_built = false;
	file_is_new = false;

	if (!find (pathstr, must_exist, is_new)) {
		return -1;
	}

	if (is_new && must_exist) {
		return -1;
	}

	return 0;
}


string
AudioFileSource::peak_path (string audio_path)
{
	return _session.peak_path_from_audio_path (audio_path);
}

string
AudioFileSource::old_peak_path (string audio_path)
{
	/* XXX hardly bombproof! fix me */

	struct stat stat_file;
	struct stat stat_mount;

	string mp = mountpoint (audio_path);

	stat (audio_path.c_str(), &stat_file);
	stat (mp.c_str(), &stat_mount);

	char buf[32];
#ifdef __APPLE__
	snprintf (buf, sizeof (buf), "%u-%u-%d.peak", stat_mount.st_ino, stat_file.st_ino, channel);
#else
	snprintf (buf, sizeof (buf), "%ld-%ld-%d.peak", stat_mount.st_ino, stat_file.st_ino, channel);
#endif

	string res = peak_dir;
	res += buf;

	return res;
}

bool
AudioFileSource::get_soundfile_info (string path, SoundFileInfo& _info, string& error_msg)
{
#ifdef HAVE_COREAUDIO
	if (CoreAudioSource::get_soundfile_info (path, _info, error_msg) == 0) {
		return true;
	}
#endif // HAVE_COREAUDIO

	if (SndFileSource::get_soundfile_info (path, _info, error_msg) != 0) {
		return true;
	}

	return false;
}

XMLNode&
AudioFileSource::get_state ()
{
	XMLNode& root (AudioSource::get_state());
	char buf[16];
	snprintf (buf, sizeof (buf), "0x%x", (int)_flags);
	root.add_property ("flags", buf);
	return root;
}

int
AudioFileSource::set_state (const XMLNode& node)
{
	const XMLProperty* prop;

	if (AudioSource::set_state (node)) {
		return -1;
	}

	if ((prop = node.property (X_("flags"))) != 0) {

		int ival;
		sscanf (prop->value().c_str(), "0x%x", &ival);
		_flags = Flag (ival);

	} else {

		_flags = Flag (0);

	}

	if ((prop = node.property (X_("name"))) != 0) {
		_is_embedded = AudioFileSource::determine_embeddedness (prop->value());
	} else {
		_is_embedded = false;
	}

	if ((prop = node.property (X_("destructive"))) != 0) {
		/* old style, from the period when we had DestructiveFileSource */
		_flags = Flag (_flags | Destructive);
	}

	return 0;
}

void
AudioFileSource::mark_for_remove ()
{
	if (!writable()) {
		return;
	}

	_flags = Flag (_flags | Removable | RemoveAtDestroy);
}

void
AudioFileSource::mark_streaming_write_completed ()
{
	if (!writable()) {
		return;
	}

	Glib::Mutex::Lock lm (_lock);

	next_peak_clear_should_notify = true;

	if (_peaks_built || pending_peak_builds.empty()) {
		_peaks_built = true;
		PeaksReady (); /* EMIT SIGNAL */
	}
}

void
AudioFileSource::mark_take (string id)
{
	if (writable()) {
		_take_id = id;
	}
}

int
AudioFileSource::move_to_trash (const string trash_dir_name)
{
	if (is_embedded()) {
		cerr << "tried to move an embedded region to trash" << endl;
		return -1;
	}

	string newpath;

	if (!writable()) {
		return -1;
	}

	/* don't move the file across filesystems, just
	   stick it in the `trash_dir_name' directory
	   on whichever filesystem it was already on.
	*/

	newpath = Glib::path_get_dirname (_path);
	newpath = Glib::path_get_dirname (newpath);

	newpath += '/';
	newpath += trash_dir_name;
	newpath += '/';
	newpath += Glib::path_get_basename (_path);

	if (access (newpath.c_str(), F_OK) == 0) {

		/* the new path already exists, try versioning */
		
		char buf[PATH_MAX+1];
		int version = 1;
		string newpath_v;

		snprintf (buf, sizeof (buf), "%s.%d", newpath.c_str(), version);
		newpath_v = buf;

		while (access (newpath_v.c_str(), F_OK) == 0 && version < 999) {
			snprintf (buf, sizeof (buf), "%s.%d", newpath.c_str(), ++version);
			newpath_v = buf;
		}
		
		if (version == 999) {
			error << string_compose (_("there are already 1000 files with names like %1; versioning discontinued"),
					  newpath)
			      << endmsg;
		} else {
			newpath = newpath_v;
		}

	} else {

		/* it doesn't exist, or we can't read it or something */

	}

	if (::rename (_path.c_str(), newpath.c_str()) != 0) {
		error << string_compose (_("cannot rename audio file source from %1 to %2 (%3)"),
				  _path, newpath, strerror (errno))
		      << endmsg;
		return -1;
	}

	if (::unlink (peakpath.c_str()) != 0) {
		error << string_compose (_("cannot remove peakfile %1 for %2 (%3)"),
				  peakpath, _path, strerror (errno))
		      << endmsg;
		/* try to back out */
		rename (newpath.c_str(), _path.c_str());
		return -1;
	}
	    
	_path = newpath;
	peakpath = "";
	
	/* file can not be removed twice, since the operation is not idempotent */

	_flags = Flag (_flags & ~(RemoveAtDestroy|Removable|RemovableIfEmpty));

	return 0;
}

bool
AudioFileSource::find (string pathstr, bool must_exist, bool& isnew)
{
	string::size_type pos;
	bool ret = false;

	isnew = false;

	/* clean up PATH:CHANNEL notation so that we are looking for the correct path */

	if ((pos = pathstr.find_last_of (':')) == string::npos) {
		pathstr = pathstr;
	} else {
		pathstr = pathstr.substr (0, pos);
	}

	if (pathstr[0] != '/') {

		/* non-absolute pathname: find pathstr in search path */

		vector<string> dirs;
		int cnt;
		string fullpath;
		string keeppath;

		if (search_path.length() == 0) {
			error << _("FileSource: search path not set") << endmsg;
			goto out;
		}

		split (search_path, dirs, ':');

		cnt = 0;
		
		for (vector<string>::iterator i = dirs.begin(); i != dirs.end(); ++i) {

			fullpath = *i;
			if (fullpath[fullpath.length()-1] != '/') {
				fullpath += '/';
			}
			fullpath += pathstr;
			
			if (access (fullpath.c_str(), R_OK) == 0) {
				keeppath = fullpath;
				++cnt;
			} 
		}

		if (cnt > 1) {

			error << string_compose (_("FileSource: \"%1\" is ambigous when searching %2\n\t"), pathstr, search_path) << endmsg;
			goto out;

		} else if (cnt == 0) {

			if (must_exist) {
				error << string_compose(_("Filesource: cannot find required file (%1): while searching %2"), pathstr, search_path) << endmsg;
				goto out;
			} else {
				isnew = true;
			}
		}
		
		_name = pathstr;
		_path = keeppath;
		ret = true;

	} else {
		
		/* external files and/or very very old style sessions include full paths */
		
		_path = pathstr;
		if (is_embedded()) {
			_name = pathstr;
		} else {
			_name = pathstr.substr (pathstr.find_last_of ('/') + 1);
		}
		
		if (access (_path.c_str(), R_OK) != 0) {

			/* file does not exist or we cannot read it */

			if (must_exist) {
				error << string_compose(_("Filesource: cannot find required file (%1): %2"), _path, strerror (errno)) << endmsg;
				goto out;
			}
			
			if (errno != ENOENT) {
				error << string_compose(_("Filesource: cannot check for existing file (%1): %2"), _path, strerror (errno)) << endmsg;
				goto out;
			}
			
			/* a new file */

			isnew = true;
			ret = true;

		} else {
			
			/* already exists */

			ret = true;
		}
	}
	
  out:
	return ret;
}

void
AudioFileSource::set_search_path (string p)
{
	search_path = p;
}

void
AudioFileSource::set_header_position_offset (nframes_t offset)
{
	header_position_offset = offset;
	HeaderPositionOffsetChanged ();
}

void
AudioFileSource::set_timeline_position (int64_t pos)
{
	timeline_position = pos;
}

void
AudioFileSource::set_allow_remove_if_empty (bool yn)
{
	if (!writable()) {
		return;
	}

	if (yn) {
		_flags = Flag (_flags | RemovableIfEmpty);
	} else {
		_flags = Flag (_flags & ~RemovableIfEmpty);
	}
}

int
AudioFileSource::set_name (string newname, bool destructive)
{
	Glib::Mutex::Lock lm (_lock);
	string oldpath = _path;
	string newpath = Session::change_audio_path_by_name (oldpath, _name, newname, destructive);

	if (newpath.empty()) {
		error << string_compose (_("programming error: %1"), "cannot generate a changed audio path") << endmsg;
		return -1;
	}

	// Test whether newpath exists, if yes notify the user but continue. 
	if (access(newpath.c_str(),F_OK) == 0) {
		error << _("Programming error! Ardour tried to rename a file over another file! It's safe to continue working, but please report this to the developers.") << endmsg;
		return -1;
	}

	if (rename (oldpath.c_str(), newpath.c_str()) != 0) {
		error << string_compose (_("cannot rename audio file for %1 to %2"), _name, newpath) << endmsg;
		return -1;
	}

	_name = Glib::path_get_basename (newpath);
	_path = newpath;

	return rename_peakfile (peak_path (_path));
}

bool
AudioFileSource::is_empty (Session& s, string path)
{
	bool ret = false;
	boost::shared_ptr<AudioFileSource> afs = boost::dynamic_pointer_cast<AudioFileSource> (SourceFactory::createReadable (s, path, NoPeakFile, false));

	if (afs) {
		ret = (afs->length() == 0);
	}

	return ret;
}

int
AudioFileSource::setup_peakfile ()
{
	if (!(_flags & NoPeakFile)) {
		return initialize_peakfile (file_is_new, _path);
	} else {
		return 0;
	}
}
