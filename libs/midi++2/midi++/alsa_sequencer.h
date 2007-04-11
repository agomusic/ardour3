/*
    Copyright (C) 2004 Paul Davis 
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

#ifndef __alsa_sequencer_midiport_h__
#define __alsa_sequencer_midiport_h__

#include <vector>
#include <string>

#include <fcntl.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <midi++/port.h>

namespace MIDI {

class ALSA_SequencerMidiPort : public Port

{
  public:
	ALSA_SequencerMidiPort (PortRequest &req);
	virtual ~ALSA_SequencerMidiPort ();

	/* select(2)/poll(2)-based I/O */

	virtual int selectable() const;

  protected:
	/* Direct I/O */
	
	int write (byte *msg, size_t msglen);	
	int read (byte *buf, size_t max);

  private:
	snd_midi_event_t *decoder, *encoder;
	int port_id;
	snd_seq_event_t SEv;

	int CreatePorts(PortRequest &req);

	static int init_client (std::string name);
	static snd_seq_t* seq;
};

}; /* namespace MIDI */

#endif // __alsa_sequencer_midiport_h__

