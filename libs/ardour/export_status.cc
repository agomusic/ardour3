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

#include <ardour/export_status.h>

namespace ARDOUR
{

ExportStatus::ExportStatus ()
{
	init();
}

void
ExportStatus::init ()
{
	stop = false;
	running = false;
	_aborted = false;
	
	stage = export_None;
	progress = 0.0;
	
	total_timespans = 0;
	timespan = 0;
	
	total_channel_configs = 0;
	channel_config = 0;
	
	total_formats = 0;
	format = 0;
}

} // namespace ARDOUR
