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

#include <glibmm/miscutils.h>

#include <ardour/control_protocol_search_path.h>
#include <ardour/directory_names.h>
#include <ardour/filesystem_paths.h>

namespace {
	const char * const surfaces_env_variable_name = "ARDOUR_SURFACES_PATH";
} // anonymous

namespace ARDOUR {

SearchPath
control_protocol_search_path ()
{
	bool surfaces_path_defined = false;
	SearchPath spath_env(Glib::getenv(surfaces_env_variable_name, surfaces_path_defined));
	
	if (surfaces_path_defined)
	{
		return spath_env;
	}

	SearchPath spath(user_config_directory ());

	spath += system_module_directory ();

	spath.add_subdirectory_to_paths(surfaces_dir_name);

	return spath;
}

} // namespace ARDOUR
