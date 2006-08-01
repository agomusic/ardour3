/*
    Copyright (C) 2006 Paul Davis 
	By Dave Robillard, 2006

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
#include <pbd/error.h>
#include <sigc++/retype.h>
#include <sigc++/retype_return.h>
#include <sigc++/bind.h>

#include <ardour/midi_track.h>
#include <ardour/midi_diskstream.h>
#include <ardour/session.h>
#include <ardour/redirect.h>
#include <ardour/midi_region.h>
#include <ardour/midi_source.h>
#include <ardour/route_group_specialized.h>
#include <ardour/insert.h>
#include <ardour/midi_playlist.h>
#include <ardour/panner.h>
#include <ardour/utils.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

MidiTrack::MidiTrack (Session& sess, string name, Route::Flag flag, TrackMode mode)
	: Track (sess, name, flag, mode, MIDI)
{
	MidiDiskstream::Flag dflags = MidiDiskstream::Flag (0);

	if (_flags & Hidden) {
		dflags = MidiDiskstream::Flag (dflags | MidiDiskstream::Hidden);
	} else {
		dflags = MidiDiskstream::Flag (dflags | MidiDiskstream::Recordable);
	}

	if (mode == Destructive) {
		dflags = MidiDiskstream::Flag (dflags | MidiDiskstream::Destructive);
	}

	MidiDiskstream* ds = new MidiDiskstream (_session, name, dflags);
	
	_declickable = true;
	_freeze_record.state = NoFreeze;
	_saved_meter_point = _meter_point;
	_mode = mode;

	set_diskstream (*ds);
}

MidiTrack::MidiTrack (Session& sess, const XMLNode& node)
	: Track (sess, node)
{
	_freeze_record.state = NoFreeze;
	set_state (node);
	_declickable = true;
	_saved_meter_point = _meter_point;
}

MidiTrack::~MidiTrack ()
{
	if (_diskstream) {
		_diskstream->unref();
	}
}


int
MidiTrack::set_diskstream (MidiDiskstream& ds)
{
	if (_diskstream) {
		_diskstream->unref();
	}

	_diskstream = &ds.ref();
	_diskstream->set_io (*this);
	_diskstream->set_destructive (_mode == Destructive);

	_diskstream->set_record_enabled (false);
	//_diskstream->monitor_input (false);

	ic_connection.disconnect();
	ic_connection = input_changed.connect (mem_fun (*_diskstream, &MidiDiskstream::handle_input_change));

	DiskstreamChanged (); /* EMIT SIGNAL */

	return 0;
}	

int 
MidiTrack::use_diskstream (string name)
{
	MidiDiskstream *dstream;

	if ((dstream = dynamic_cast<MidiDiskstream*>(_session.diskstream_by_name (name))) == 0) {
	  error << string_compose(_("MidiTrack: midi diskstream \"%1\" not known by session"), name) << endmsg;
		return -1;
	}
	
	return set_diskstream (*dstream);
}

int 
MidiTrack::use_diskstream (const PBD::ID& id)
{
	MidiDiskstream *dstream;

	if ((dstream = dynamic_cast<MidiDiskstream*>(_session.diskstream_by_id (id))) == 0) {
	  	error << string_compose(_("MidiTrack: midi diskstream \"%1\" not known by session"), id) << endmsg;
		return -1;
	}
	
	return set_diskstream (*dstream);
}

bool
MidiTrack::record_enabled () const
{
	return _diskstream->record_enabled ();
}

void
MidiTrack::set_record_enable (bool yn, void *src)
{
	if (_freeze_record.state == Frozen) {
		return;
	}
#if 0
	if (_mix_group && src != _mix_group && _mix_group->is_active()) {
		_mix_group->apply (&MidiTrack::set_record_enable, yn, _mix_group);
		return;
	}

	/* keep track of the meter point as it was before we rec-enabled */

	if (!diskstream->record_enabled()) {
		_saved_meter_point = _meter_point;
	}
	
	diskstream->set_record_enabled (yn, src);

	if (diskstream->record_enabled()) {
		set_meter_point (MeterInput, this);
	} else {
		set_meter_point (_saved_meter_point, this);
	}

	if (_session.get_midi_feedback()) {
		_midi_rec_enable_control.send_feedback (record_enabled());
	}
#endif
}

MidiDiskstream&
MidiTrack::midi_diskstream() const
{
	return *dynamic_cast<MidiDiskstream*>(_diskstream);
}

int
MidiTrack::set_state (const XMLNode& node)
{
	const XMLProperty *prop;
	XMLNodeConstIterator iter;

	if (Route::set_state (node)) {
		return -1;
	}

	if ((prop = node.property (X_("mode"))) != 0) {
		if (prop->value() == X_("normal")) {
			_mode = Normal;
		} else if (prop->value() == X_("destructive")) {
			_mode = Destructive;
		} else {
			warning << string_compose ("unknown midi track mode \"%1\" seen and ignored", prop->value()) << endmsg;
			_mode = Normal;
		}
	} else {
		_mode = Normal;
	}

	if ((prop = node.property ("diskstream-id")) == 0) {
		
		/* some old sessions use the diskstream name rather than the ID */

		if ((prop = node.property ("diskstream")) == 0) {
			fatal << _("programming error: MidiTrack given state without diskstream!") << endmsg;
			/*NOTREACHED*/
			return -1;
		}

		if (use_diskstream (prop->value())) {
			return -1;
		}

	} else {
		
		PBD::ID id (prop->value());
		
		if (use_diskstream (id)) {
			return -1;
		}
	}


	XMLNodeList nlist;
	XMLNodeConstIterator niter;
	XMLNode *child;

	nlist = node.children();
	for (niter = nlist.begin(); niter != nlist.end(); ++niter){
		child = *niter;

		if (child->name() == X_("remote_control")) {
			if ((prop = child->property (X_("id"))) != 0) {
				int32_t x;
				sscanf (prop->value().c_str(), "%d", &x);
				set_remote_control_id (x);
			}
		}
	}

	pending_state = const_cast<XMLNode*> (&node);

	_session.StateReady.connect (mem_fun (*this, &MidiTrack::set_state_part_two));

	return 0;
}

XMLNode& 
MidiTrack::state(bool full_state)
{
	XMLNode& root (Route::state(full_state));
	XMLNode* freeze_node;
	char buf[64];

	if (_freeze_record.playlist) {
		XMLNode* inode;

		freeze_node = new XMLNode (X_("freeze-info"));
		freeze_node->add_property ("playlist", _freeze_record.playlist->name());
		snprintf (buf, sizeof (buf), "%d", (int) _freeze_record.state);
		freeze_node->add_property ("state", buf);

		for (vector<FreezeRecordInsertInfo*>::iterator i = _freeze_record.insert_info.begin(); i != _freeze_record.insert_info.end(); ++i) {
			inode = new XMLNode (X_("insert"));
			(*i)->id.print (buf);
			inode->add_property (X_("id"), buf);
			inode->add_child_copy ((*i)->state);
		
			freeze_node->add_child_nocopy (*inode);
		}

		root.add_child_nocopy (*freeze_node);
	}

	/* Alignment: act as a proxy for the diskstream */
	
	XMLNode* align_node = new XMLNode (X_("alignment"));
	switch (_diskstream->alignment_style()) {
	case ExistingMaterial:
		snprintf (buf, sizeof (buf), X_("existing"));
		break;
	case CaptureTime:
		snprintf (buf, sizeof (buf), X_("capture"));
		break;
	}
	align_node->add_property (X_("style"), buf);
	root.add_child_nocopy (*align_node);

	XMLNode* remote_control_node = new XMLNode (X_("remote_control"));
	snprintf (buf, sizeof (buf), "%d", _remote_control_id);
	remote_control_node->add_property (X_("id"), buf);
	root.add_child_nocopy (*remote_control_node);

	switch (_mode) {
	case Normal:
		root.add_property (X_("mode"), X_("normal"));
		break;
	case Destructive:
		root.add_property (X_("mode"), X_("destructive"));
		break;
	}

	/* we don't return diskstream state because we don't
	   own the diskstream exclusively. control of the diskstream
	   state is ceded to the Session, even if we create the
	   diskstream.
	*/

	_diskstream->id().print (buf);
	root.add_property ("diskstream-id", buf);

	return root;
}

void
MidiTrack::set_state_part_two ()
{
	XMLNode* fnode;
	XMLProperty* prop;
	LocaleGuard lg (X_("POSIX"));

	/* This is called after all session state has been restored but before
	   have been made ports and connections are established.
	*/

	if (pending_state == 0) {
		return;
	}

	if ((fnode = find_named_node (*pending_state, X_("freeze-info"))) != 0) {

		
		_freeze_record.have_mementos = false;
		_freeze_record.state = Frozen;
		
		for (vector<FreezeRecordInsertInfo*>::iterator i = _freeze_record.insert_info.begin(); i != _freeze_record.insert_info.end(); ++i) {
			delete *i;
		}
		_freeze_record.insert_info.clear ();
		
		if ((prop = fnode->property (X_("playlist"))) != 0) {
			Playlist* pl = _session.playlist_by_name (prop->value());
			if (pl) {
				_freeze_record.playlist = dynamic_cast<MidiPlaylist*> (pl);
			} else {
				_freeze_record.playlist = 0;
				_freeze_record.state = NoFreeze;
			return;
			}
		}
		
		if ((prop = fnode->property (X_("state"))) != 0) {
			_freeze_record.state = (FreezeState) atoi (prop->value().c_str());
		}
		
		XMLNodeConstIterator citer;
		XMLNodeList clist = fnode->children();
		
		for (citer = clist.begin(); citer != clist.end(); ++citer) {
			if ((*citer)->name() != X_("insert")) {
				continue;
			}
			
			if ((prop = (*citer)->property (X_("id"))) == 0) {
				continue;
			}
			
			FreezeRecordInsertInfo* frii = new FreezeRecordInsertInfo (*((*citer)->children().front()),
										   boost::shared_ptr<Insert>());
			frii->id = prop->value ();
			_freeze_record.insert_info.push_back (frii);
		}
	}

	/* Alignment: act as a proxy for the diskstream */

	if ((fnode = find_named_node (*pending_state, X_("alignment"))) != 0) {

		if ((prop = fnode->property (X_("style"))) != 0) {
			if (prop->value() == "existing") {
				_diskstream->set_persistent_align_style (ExistingMaterial);
			} else if (prop->value() == "capture") {
				_diskstream->set_persistent_align_style (CaptureTime);
			}
		}
	}
	return;
}	

uint32_t
MidiTrack::n_process_buffers ()
{
	return max ((uint32_t) _diskstream->n_channels(), redirect_max_outs);
}

void
MidiTrack::passthru_silence (jack_nframes_t start_frame, jack_nframes_t end_frame, jack_nframes_t nframes, jack_nframes_t offset, int declick, bool meter)
{
	uint32_t nbufs = n_process_buffers ();
	process_output_buffers (_session.get_silent_buffers (nbufs), nbufs, start_frame, end_frame, nframes, offset, true, declick, meter);
}

int 
MidiTrack::no_roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame, jack_nframes_t offset, 
		     bool session_state_changing, bool can_record, bool rec_monitors_input)
{
	if (n_outputs() == 0) {
		return 0;
	}

	if (!_active) {
		silence (nframes, offset);
		return 0;
	}

	if (session_state_changing) {

		/* XXX is this safe to do against transport state changes? */

		passthru_silence (start_frame, end_frame, nframes, offset, 0, false);
		return 0;
	}

	midi_diskstream().check_record_status (start_frame, nframes, can_record);

	bool send_silence;
	
	if (_have_internal_generator) {
		/* since the instrument has no input streams,
		   there is no reason to send any signal
		   into the route.
		*/
		send_silence = true;
	} else {

		if (_session.get_auto_input()) {
			if (Config->get_use_sw_monitoring()) {
				send_silence = false;
			} else {
				send_silence = true;
			}
		} else {
			if (_diskstream->record_enabled()) {
				if (Config->get_use_sw_monitoring()) {
					send_silence = false;
				} else {
					send_silence = true;
				}
			} else {
				send_silence = true;
			}
		}
	}

	apply_gain_automation = false;

	if (send_silence) {
		
		/* if we're sending silence, but we want the meters to show levels for the signal,
		   meter right here.
		*/
		
		if (_have_internal_generator) {
			passthru_silence (start_frame, end_frame, nframes, offset, 0, true);
		} else {
			if (_meter_point == MeterInput) {
				just_meter_input (start_frame, end_frame, nframes, offset);
			}
			passthru_silence (start_frame, end_frame, nframes, offset, 0, false);
		}

	} else {
	
		/* we're sending signal, but we may still want to meter the input. 
		 */

		passthru (start_frame, end_frame, nframes, offset, 0, (_meter_point == MeterInput));
	}

	return 0;
}

int
MidiTrack::roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame, jack_nframes_t offset, int declick,
		  bool can_record, bool rec_monitors_input)
{
	return 0;
}

int
MidiTrack::silent_roll (jack_nframes_t nframes, jack_nframes_t start_frame, jack_nframes_t end_frame, jack_nframes_t offset, 
			 bool can_record, bool rec_monitors_input)
{
	if (n_outputs() == 0 && _redirects.empty()) {
		return 0;
	}

	if (!_active) {
		silence (nframes, offset);
		return 0;
	}

	_silent = true;
	apply_gain_automation = false;

	silence (nframes, offset);

	return midi_diskstream().process (_session.transport_frame() + offset, nframes, offset, can_record, rec_monitors_input);
}

int
MidiTrack::set_name (string str, void *src)
{
	int ret;

	if (record_enabled() && _session.actively_recording()) {
		/* this messes things up if done while recording */
		return -1;
	}

	if (_diskstream->set_name (str)) {
		return -1;
	}

	/* save state so that the statefile fully reflects any filename changes */

	if ((ret = IO::set_name (str, src)) == 0) {
		_session.save_state ("");
	}
	return ret;
}

int
MidiTrack::export_stuff (vector<unsigned char*>& buffers, char * workbuf, uint32_t nbufs, jack_nframes_t start, jack_nframes_t nframes)
{
	return 0;
}

void
MidiTrack::set_latency_delay (jack_nframes_t longest_session_latency)
{
	Route::set_latency_delay (longest_session_latency);
	_diskstream->set_roll_delay (_roll_delay);
}

void
MidiTrack::bounce (InterThreadInfo& itt)
{
	//vector<MidiSource*> srcs;
	//_session.write_one_midi_track (*this, 0, _session.current_end_frame(), false, srcs, itt);
}


void
MidiTrack::bounce_range (jack_nframes_t start, jack_nframes_t end, InterThreadInfo& itt)
{
	//vector<MidiSource*> srcs;
	//_session.write_one_midi_track (*this, start, end, false, srcs, itt);
}

void
MidiTrack::freeze (InterThreadInfo& itt)
{
}

void
MidiTrack::unfreeze ()
{
	_freeze_record.state = UnFrozen;
	FreezeChange (); /* EMIT SIGNAL */
}

void
MidiTrack::set_mode (TrackMode m)
{
	if (_diskstream) {
		if (_mode != m) {
			_mode = m;
			_diskstream->set_destructive (m == Destructive);
			ModeChanged();
		}
	}
}
