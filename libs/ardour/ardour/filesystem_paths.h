/*
    Copyright (C) 2007 Tim Mayberry 

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

#ifndef ARDOUR_FILESYSTEM_PATHS_INCLUDED
#define ARDOUR_FILESYSTEM_PATHS_INCLUDED

#include <pbd/filesystem.h>
#include <pbd/search_path.h>

namespace ARDOUR {

	using namespace PBD;

	/**
	 * @return the path to the directory used to store user specific ardour
	 * configuration files.
	 */
	sys::path user_config_directory ();

	/**
	 * @return the path to the directory that contains the system wide ardour
	 * modules.
	 */
	sys::path ardour_module_directory ();

	SearchPath config_search_path ();

} // namespace ARDOUR

#endif
