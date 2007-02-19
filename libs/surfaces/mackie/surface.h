#ifndef mackie_surface_h
#define mackie_surface_h

#include "controls.h"
#include "types.h"
#include <stdint.h>

namespace Mackie
{

class MackieButtonHandler;

/**
	This represents an entire control surface, made up of Groups,
	Strips and Controls. There are several collections for
	ease of addressing in different ways, but only one collection
	has definitive ownership.

	It handles mapping button ids to press_ and release_ calls.

	There are various emulations of the Mackie around, so specific
	emulations will inherit from this to change button mapping, or 
	have 7 fader channels instead of 8, or whatever.

	Currently there are BcfSurface and MackieSurface.

	TODO maybe make Group inherit from Control, for ease of ownership.
*/
class Surface
{
public:
	/**
		A Surface can be made up of multiple units. eg one Mackie MCU plus
		one or more Mackie MCU extenders.
		
		\param max_strips is the number of strips for the entire surface.
		\param unit_strips is the number of strips per unit.
	*/
	Surface( uint32_t max_strips, uint32_t unit_strips = 8 );
	virtual ~Surface();

	/// Calls the virtual initialisation methods. This *must* be called after
	/// construction, because c++ is too dumb to call virtual methods from
	/// inside a constructor
	void init();

	typedef std::vector<Control*> Controls;
	
	/// This collection has ownership of all the controls
	Controls controls;

	/**
		These are alternative addressing schemes
		They use maps because the indices aren't always
		0-based.
	*/
	std::map<int,Control*> faders;
	std::map<int,Control*> pots;
	std::map<int,Control*> buttons;
	std::map<int,Control*> leds;

	/// no strip controls in here because they usually
	/// have the same names.
	std::map<std::string,Control*> controls_by_name;

	/// The collection of all numbered strips. No master
	/// strip in here.
	typedef std::vector<Strip*> Strips;
	Strips strips;

	/// This collection owns the groups
	typedef std::map<std::string,Group*> Groups;
	Groups groups;

	uint32_t max_strips() const
	{
		return _max_strips;
	}
	
	/// map button ids to calls to press_ and release_ in mbh
	virtual void handle_button( MackieButtonHandler & mbh, ButtonState bs, Button & button ) = 0;
	
protected:
	virtual void init_controls() = 0;
	virtual void init_strips( uint32_t max_strips, uint32_t unit_strips );

private:
	uint32_t _max_strips;
	uint32_t _unit_strips;
};

}

#endif
