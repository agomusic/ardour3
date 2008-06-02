#include <cstdio>
#include <fcntl.h>

#include <pbd/error.h>
#include <pbd/textreceiver.h>

Transmitter error (Transmitter::Error);
Transmitter info (Transmitter::Info);
Transmitter warning (Transmitter::Warning);
Transmitter fatal (Transmitter::Fatal);
TextReceiver text_receiver ("mmctest");

#include "midi++/port.h"
#include "midi++/manager.h"

using namespace MIDI;

Port *port;
PortRequest midi_device;

int 
setup_midi ()

{
	midi_device.devname = "/dev/snd/midiC0D0";
	midi_device.tagname = "trident";
	midi_device.mode = O_RDWR;
	midi_device.type = Port::ALSA_RawMidi;

	if ((port = MIDI::Manager::instance()->add_port (midi_device)) == 0) {
		info << "MIDI port is not valid" << endmsg;
		return -1;
	} 

	return 0;
}

main (int argc, char *argv[]) 

{
	byte buf[1];
	
	text_receiver.listen_to (error);
	text_receiver.listen_to (info);
	text_receiver.listen_to (fatal);
	text_receiver.listen_to (warning);

	if (setup_midi ()) {
		exit (1);
	}

	port->input()->trace (true, &cout);

	while (1) {
		if (port->read (buf, 1) < 0) {
			error << "cannot read byte"
			      << endmsg;
			break;
		} 
	}
}


