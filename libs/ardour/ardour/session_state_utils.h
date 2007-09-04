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

#ifndef ARDOUR_SESSION_STATE_UTILS_INCLUDED
#define ARDOUR_SESSION_STATE_UTILS_INCLUDED

#include <vector>
#include <string>

#include <pbd/filesystem.h>

namespace ARDOUR {

using std::string;
using std::vector;
using namespace PBD;

/**
 * Attempt to create a backup copy of a file.
 *
 * A copy of the file is created in the same directory using 
 * the same filename with the backup suffix appended.
 *
 * @return true if successful, false otherwise.
 */
bool create_backup_file (const sys::path & file_path);

/**
 * Get the absolute paths to all state files in the directory 
 * at path directory_path.
 *
 * @param directory_path The absolute path to a directory.
 * @param result vector to contain resulting state files.
 */
void get_state_files_in_directory (const sys::path & directory_path,
		vector<sys::path>& result);

/**
 * Given a vector of paths to files, return a vector containing
 * the filenames without any extension.
 *
 * @param file_paths a vector containing the file paths
 * @return a vector containing a list of file names without any 
 * filename extension.
 */
vector<string> get_file_names_no_extension (const vector<sys::path> & file_paths);

} // namespace ARDOUR

#endif
