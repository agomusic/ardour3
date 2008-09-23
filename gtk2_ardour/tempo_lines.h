/*
    Copyright (C) 2000-2007 Paul Davis 

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

#ifndef __ardour_tempo_lines_h__
#define __ardour_tempo_lines_h__

#include <map>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <ardour/tempo.h>
#include "canvas.h"
#include "simpleline.h"

typedef boost::fast_pool_allocator<
		std::pair<double, ArdourCanvas::SimpleLine>,
		boost::default_user_allocator_new_delete,
		boost::details::pool::null_mutex,
		8192>
	MapAllocator;

class TempoLines {
public:
	TempoLines(ArdourCanvas::Canvas& canvas, ArdourCanvas::Group* group);

	void tempo_map_changed();

	void draw(ARDOUR::TempoMap::BBTPointList& points, double frames_per_unit);

	void show();
	void hide();

private:
	typedef std::map<double, ArdourCanvas::SimpleLine*, std::less<double>, MapAllocator> Lines;
	Lines _lines;

	ArdourCanvas::Canvas& _canvas;
	ArdourCanvas::Group*  _group;
	double                _clean_left;
	double                _clean_right;
};

#endif /* __ardour_tempo_lines_h__ */
