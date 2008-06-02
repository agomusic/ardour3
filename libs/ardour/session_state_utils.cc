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

#include <algorithm>

#include <pbd/compose.h>
#include <pbd/error.h>
#include <pbd/file_utils.h>

#include <ardour/session_state_utils.h>
#include <ardour/filename_extensions.h>

#include "i18n.h"

namespace ARDOUR {

bool
create_backup_file (const sys::path & file_path)
{
	if (!sys::exists (file_path)) return false;

	sys::path backup_path(file_path.to_string() + backup_suffix);

	try
	{
		sys::copy_file (file_path, backup_path);
	}
	catch(sys::filesystem_error& ex)
	{
		error << string_compose (_("Unable to create a backup copy of file %1 (%2)"),
				file_path.to_string(), ex.what())
			<< endmsg;
		return false;
	}
	return true;
}

void
get_state_files_in_directory (const sys::path & directory_path,
		vector<sys::path> & result)
{
	Glib::PatternSpec state_file_pattern('*' + string(statefile_suffix));
	
	find_matching_files_in_directory (directory_path, state_file_pattern,
			result);
}

vector<string>
get_file_names_no_extension (const vector<sys::path> & file_paths)
{
	vector<string> result;

	std::transform (file_paths.begin(), file_paths.end(),
			std::back_inserter(result), sys::basename);

	sort (result.begin(), result.end(), std::less<string>());

	return result;
}

} // namespace ARDOUR
