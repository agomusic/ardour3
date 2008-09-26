/*
    Copyright (C) 2008 Paul Davis
    Author: Sakari Bergen

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

#ifndef __ardour_location_importer_h__
#define __ardour_location_importer_h__

#include <ardour/element_importer.h>
#include <ardour/element_import_handler.h>

#include <boost/shared_ptr.hpp>

#include <pbd/xml++.h>
#include <ardour/location.h>
#include <ardour/types.h>

namespace ARDOUR {

class LocationImportHandler : public ElementImportHandler
{
  public:
	LocationImportHandler (XMLTree const & source, Session & session);
	string get_info () const;
};

class LocationImporter : public ElementImporter
{
  public:
	LocationImporter (XMLTree const & source, Session & session, LocationImportHandler & handler, XMLNode const & node);
	~LocationImporter ();

	string get_info () const;
	bool prepare_move ();
	void cancel_move ();
	void move ();

  private:
	LocationImportHandler & handler;
	XMLNode                 xml_location;
	Location *              location;
	
	void parse_xml ();
};

} // namespace ARDOUR

#endif
