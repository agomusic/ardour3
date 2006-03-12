/*
    Copyright (C) 2005 Paul Davis 

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

    $Id$
*/

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <ardour/ardour.h>

#include "i18n.h"
#include "glade_path.h"

#include <iostream>

std::string
GladePath::path(const std::string& glade_file)
{
    std::string full_path;
   
    full_path = ARDOUR::find_data_file(glade_file, "glade");
    return full_path;
}
