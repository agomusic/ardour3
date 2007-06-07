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

#include <pbd/tokenizer.h>
#include <pbd/search_path.h>
#include <pbd/error.h>

namespace {

#ifdef WIN32
const char * const path_delimiter = ";";
#else
const char * const path_delimiter = ":";
#endif

}

namespace PBD {

SearchPath::SearchPath ()
{

}

SearchPath::SearchPath (const string& path)
{
	vector<sys::path> tmp;

	if(!tokenize ( path, string(path_delimiter), std::back_inserter (tmp)))
	{
		// log warning(info perhaps?) that the path is empty 
		warning << "SearchPath contains no tokens" << endmsg;

	}

	add_directories (tmp);
}

SearchPath::SearchPath (const vector<sys::path>& paths)
{
	add_directories (paths);
}

SearchPath::SearchPath (const SearchPath& other)
	: m_dirs(other.m_dirs)
{

}

void
SearchPath::add_directory (const sys::path& directory_path)
{
	// test for existance and warn etc?
	m_dirs.push_back(directory_path);
}

void
SearchPath::add_directories (const vector<sys::path>& paths)
{
	for(vector<sys::path>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
		add_directory (*i);
	}
}

const string
SearchPath::get_string () const
{
	string path;

	for (vector<sys::path>::const_iterator i = m_dirs.begin(); i != m_dirs.end(); ++i) {
		path += (*i).to_string();
		path += path_delimiter;
	}

	path = path.substr (0, path.length() - 1); // drop final separator

	return path;
}

SearchPath&
SearchPath::operator= (const SearchPath& path)
{
	m_dirs = path.m_dirs;
	return *this;
}

SearchPath& 
SearchPath::operator+= (const SearchPath& spath)
{
	m_dirs.insert(m_dirs.end(), spath.m_dirs.begin(), spath.m_dirs.end());
	return *this;
}

SearchPath& 
SearchPath::operator+= (const sys::path& directory_path)
{
	add_directory (directory_path);
	return *this;
}

SearchPath& 
SearchPath::operator+ (const sys::path& directory_path)
{
	add_directory (directory_path);
	return *this;
}

SearchPath& 
SearchPath::operator+ (const SearchPath& spath)
{
	// concatenate paths into new SearchPath
	m_dirs.insert(m_dirs.end(), spath.m_dirs.begin(), spath.m_dirs.end());
	return *this;
}

SearchPath&
SearchPath::add_subdirectory_to_paths (const string& subdir)
{
	vector<sys::path> tmp;
	string directory_path;

	for (vector<sys::path>::iterator i = m_dirs.begin(); i != m_dirs.end(); ++i)
	{
		// should these new paths just be added to the end of 
		// the search path rather than replace?
		*i /= subdir;
	}
	
	return *this;
}
	
SearchPath&
SearchPath::operator/= (const string& subdir)
{
	return add_subdirectory_to_paths (subdir);
}

} // namespace PBD
