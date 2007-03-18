/*
	Copyright (C) 2006,2007 John Anderson

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
#include "mackie_midi_builder.h"

#include <typeinfo>
#include <sstream>
#include <iomanip>

#include "controls.h"
#include "midi_byte_array.h"

using namespace Mackie;
using namespace std;

MIDI::byte MackieMidiBuilder::calculate_pot_value( midi_pot_mode mode, const ControlState & state )
{
	// TODO do an exact calc for 0.50? To allow manually re-centering the port.
	
	// center on or off
	MIDI::byte retval = ( state.pos > 0.45 && state.pos < 0.55 ? 1 : 0 ) << 6;
	
	// mode
	retval |= ( mode << 4 );
	
	// value, but only if off hasn't explicitly been set
	if ( state.led_state != off )
		retval += ( int(state.pos * 10.0) + 1 ) & 0x0f; // 0b00001111
	
	return retval;
}

MidiByteArray MackieMidiBuilder::build_led_ring( const Pot & pot, const ControlState & state )
{
	return build_led_ring( pot.led_ring(), state );
}

MidiByteArray MackieMidiBuilder::build_led_ring( const LedRing & led_ring, const ControlState & state )
{
	// The other way of doing this:
	// 0x30 + pot/ring number (0-7)
	//, 0x30 + led_ring.ordinal() - 1
	return MidiByteArray ( 3
		// the control type
		, midi_pot_id
		// the id
		, 0x20 + led_ring.id()
		// the value
		, calculate_pot_value( midi_pot_mode_dot, state )
	);
}

MidiByteArray MackieMidiBuilder::build_led( const Button & button, LedState ls )
{
	return build_led( button.led(), ls );
}

MidiByteArray MackieMidiBuilder::build_led( const Led & led, LedState ls )
{
	MIDI::byte state = 0;
	switch ( ls.state() )
	{
		case LedState::on:			state = 0x7f; break;
		case LedState::off:			state = 0x00; break;
		case LedState::none:			state = 0x00; break; // actually, this should never happen.
		case LedState::flashing:	state = 0x01; break;
	}
	
	return MidiByteArray ( 3
		, midi_button_id
		, led.id()
		, state
	);
}

MidiByteArray MackieMidiBuilder::build_fader( const Fader & fader, float pos )
{
	int posi = int( 0x3fff * pos );
	
	return MidiByteArray ( 3
		, midi_fader_id | fader.id()
		// lower-order bits
		, posi & 0x7f
		// higher-order bits
		, ( posi >> 7 )
	);
}

MidiByteArray MackieMidiBuilder::zero_strip( const Strip & strip )
{
	Group::Controls::const_iterator it = strip.controls().begin();
	MidiByteArray retval;
	for (; it != strip.controls().end(); ++it )
	{
		Control & control = **it;
		if ( control.accepts_feedback() )
			retval << zero_control( control );
	}
	return retval;
}

MidiByteArray MackieMidiBuilder::zero_control( const Control & control )
{
	switch( control.type() )
	{
		case Control::type_button:
			return build_led( (Button&)control, off );
		
		case Control::type_led:
			return build_led( (Led&)control, off );
		
		case Control::type_fader:
			return build_fader( (Fader&)control, 0.0 );
		
		case Control::type_pot:
			return build_led_ring( dynamic_cast<const Pot&>( control ), off );
		
		case Control::type_led_ring:
			return build_led_ring( dynamic_cast<const LedRing&>( control ), off );
		
		default:
			ostringstream os;
			os << "Unknown control type " << control << " in Strip::zero_control";
			throw MackieControlException( os.str() );
	}
}

char translate_seven_segment( char achar )
{
	achar = toupper( achar );
	if ( achar >= 0x40 && achar <= 0x60 )
		return achar - 0x40;
	else if ( achar >= 0x21 && achar <= 0x3f )
      return achar;
	else
      return 0x00;
}

MidiByteArray MackieMidiBuilder::two_char_display( const std::string & msg, const std::string & dots )
{
	if ( msg.length() != 2 ) throw MackieControlException( "MackieMidiBuilder::two_char_display: msg must be exactly 2 characters" );
	if ( dots.length() != 2 ) throw MackieControlException( "MackieMidiBuilder::two_char_display: dots must be exactly 2 characters" );
	
	MidiByteArray bytes( 5, 0xb0, 0x4a, 0x00, 0x4b, 0x00 );
	
	// chars are understood by the surface in right-to-left order
	// could also exchange the 0x4a and 0x4b, above
	bytes[4] = translate_seven_segment( msg[0] ) + ( dots[0] == '.' ? 0x40 : 0x00 );
	bytes[2] = translate_seven_segment( msg[1] ) + ( dots[1] == '.' ? 0x40 : 0x00 );
	
	return bytes;
}

MidiByteArray MackieMidiBuilder::two_char_display( unsigned int value, const std::string & dots )
{
	ostringstream os;
	os << setfill('0') << setw(2) << value % 100;
	return two_char_display( os.str() );
}
