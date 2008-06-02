/*
    Copyright (C) 2006 Paul Davis
    Written by Taybin Rutkin

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

#include <algorithm>

#include <pbd/error.h>
#include <ardour/coreaudiosource.h>
#include <ardour/utils.h>

#include <appleutility/CAAudioFile.h>
#include <appleutility/CAStreamBasicDescription.h>

#include "i18n.h"

#include <AudioToolbox/AudioFormat.h>

using namespace std;
using namespace ARDOUR;
using namespace PBD;

CoreAudioSource::CoreAudioSource (Session& s, const XMLNode& node)
	: AudioFileSource (s, node)
{
	init ();
}

CoreAudioSource::CoreAudioSource (Session& s, const string& path, int chn, Flag flags)
	/* files created this way are never writable or removable */
	: AudioFileSource (s, path, Flag (flags & ~(Writable|Removable|RemovableIfEmpty|RemoveAtDestroy)))
{
	_channel = chn;
	init ();
}

void 
CoreAudioSource::init ()
{
	/* note that we temporarily truncated _id at the colon */
	try {
		af.Open(_path.c_str());

		CAStreamBasicDescription file_format (af.GetFileDataFormat());
		n_channels = file_format.NumberChannels();
		
		if (_channel >= n_channels) {
			error << string_compose("CoreAudioSource: file only contains %1 channels; %2 is invalid as a channel number (%3)", n_channels, _channel, name()) << endmsg;
			throw failed_constructor();
		}

		_length = af.GetNumberFrames();

		CAStreamBasicDescription client_format (file_format);

		/* set canonial form (PCM, native float packed, 32 bit, with the correct number of channels
		   and interleaved (since we plan to deinterleave ourselves)
		*/

		client_format.SetCanonical(client_format.NumberChannels(), true);
		af.SetClientFormat (client_format);

	} catch (CAXException& cax) {
		
		error << string_compose(_("CoreAudioSource: cannot open file \"%1\" for %2"), 
					_path, (writable() ? "read+write" : "reading")) << endmsg;
		throw failed_constructor ();
	}
}

CoreAudioSource::~CoreAudioSource ()
{
	GoingAway (); /* EMIT SIGNAL */
}

int
CoreAudioSource::safe_read (Sample* dst, nframes_t start, nframes_t cnt, AudioBufferList& abl) const
{
	nframes_t nread = 0;

	while (nread < cnt) {
		
		try {
			af.Seek (start+nread);
		} catch (CAXException& cax) {
			error << string_compose("CoreAudioSource: %1 to %2 (%3)", cax.mOperation, start+nread, _name.substr (1)) << endmsg;
			return -1;
		}
		
		UInt32 new_cnt = cnt - nread;
		
		abl.mBuffers[0].mDataByteSize = new_cnt * n_channels * sizeof(Sample);
		abl.mBuffers[0].mData = dst + nread;
			
		try {
			af.Read (new_cnt, &abl);
		} catch (CAXException& cax) {
			error << string_compose("CoreAudioSource: %1 (%2)", cax.mOperation, _name);
			return -1;
		}

		if (new_cnt == 0) {
			/* EOF */
			if (start+cnt == _length) {
				/* we really did hit the end */
				nread = cnt;
			}
			break;
		}

		nread += new_cnt;
	}

	if (nread < cnt) {
		return -1;
	} else {
		return 0;
	}
}
	

nframes_t
CoreAudioSource::read_unlocked (Sample *dst, nframes_t start, nframes_t cnt) const
{
	nframes_t file_cnt;
	AudioBufferList abl;

	abl.mNumberBuffers = 1;
	abl.mBuffers[0].mNumberChannels = n_channels;

	if (start > _length) {

		/* read starts beyond end of data, just memset to zero */
		
		file_cnt = 0;

	} else if (start + cnt > _length) {
		
		/* read ends beyond end of data, read some, memset the rest */
		
		file_cnt = _length - start;

	} else {
		
		/* read is entirely within data */

		file_cnt = cnt;
	}

	if (file_cnt != cnt) {
		nframes_t delta = cnt - file_cnt;
		memset (dst+file_cnt, 0, sizeof (Sample) * delta);
	}

	if (file_cnt) {

		if (n_channels == 1) {
			if (safe_read (dst, start, file_cnt, abl) == 0) {
				_read_data_count = cnt * sizeof (Sample);
				return cnt;
			}
			return 0;
		}
	}

	Sample* interleave_buf = get_interleave_buffer (file_cnt * n_channels);
	
	if (safe_read (interleave_buf, start, file_cnt, abl) != 0) {
		return 0;
	}

	_read_data_count = cnt * sizeof(float);

	Sample *ptr = interleave_buf + _channel;
	
	/* stride through the interleaved data */
	
	for (uint32_t n = 0; n < file_cnt; ++n) {
		dst[n] = *ptr;
		ptr += n_channels;
	}

	return cnt;
}

float
CoreAudioSource::sample_rate() const
{
	CAStreamBasicDescription client_asbd;

	try {
		client_asbd = af.GetClientDataFormat ();
	} catch (CAXException& cax) {
		error << string_compose("CoreAudioSource: %1 (%2)", cax.mOperation, _name);
		return 0.0;
	}

	return client_asbd.mSampleRate;
}

int
CoreAudioSource::update_header (nframes_t when, struct tm&, time_t)
{
	return 0;
}

int
CoreAudioSource::get_soundfile_info (string path, SoundFileInfo& _info, string& error_msg)
{
	FSRef ref; 
	ExtAudioFileRef af = 0;
	size_t size;
	CFStringRef name;
	int ret = -1;

	if (FSPathMakeRef ((UInt8*)path.c_str(), &ref, 0) != noErr) {
		goto out;
	}
	
	if (ExtAudioFileOpen(&ref, &af) != noErr) {
		goto out;
	}
	
	AudioStreamBasicDescription absd;
	memset(&absd, 0, sizeof(absd));
	size = sizeof(AudioStreamBasicDescription);
	if (ExtAudioFileGetProperty (af, kExtAudioFileProperty_FileDataFormat, &size, &absd) != noErr) {
		goto out;
	}
	
	_info.samplerate = absd.mSampleRate;
	_info.channels   = absd.mChannelsPerFrame;

	size = sizeof(_info.length);
	if (ExtAudioFileGetProperty(af, kExtAudioFileProperty_FileLengthFrames, &size, &_info.length) != noErr) {
		goto out;
	}
	
	size = sizeof(CFStringRef);
	if (AudioFormatGetProperty(kAudioFormatProperty_FormatName, sizeof(absd), &absd, &size, &name) != noErr) {
		goto out;
	}

	_info.format_name = CFStringRefToStdString(name);

	// XXX it would be nice to find a way to get this information if it exists

	_info.timecode = 0;
	ret = 0;
	
  out:
	ExtAudioFileDispose (af);
	return ret;
	
}
