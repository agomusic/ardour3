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

#ifndef __ardour_audio_unit_h__
#define __ardour_audio_unit_h__

#include <list>

#include <ardour/plugin.h>

#include <boost/shared_ptr.hpp>

struct ComponentDescription;

namespace ARDOUR {

class AUPlugin : public ARDOUR::Plugin
{
  public:
	AUPlugin (AudioEngine& engine, Session& session) : Plugin(engine, session) {};
	virtual ~AUPlugin () {};
};

class AUPluginInfo : public PluginInfo {
  public:
	typedef boost::shared_ptr<ComponentDescription> CompDescPtr;
	
	AUPluginInfo () { };
	~AUPluginInfo () { };

	CompDescPtr desc;

	static PluginInfoList discover ();

  private:
	friend class PluginManager;
};

typedef boost::shared_ptr<AUPluginInfo> AUPluginInfoPtr;

} // namespace ARDOUR

#endif // __ardour_audio_unit_h__