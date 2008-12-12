#include "canvas-flag.h"
#include <iostream>
#include "ardour_ui.h"

using namespace Gnome::Canvas;
using namespace std;


void 
CanvasFlag::delete_allocated_objects()
{
	if (_text) {
		delete _text;
		_text = 0;
	}
	
	if (_line) {
		delete _line;
		_line = 0;
	}
	
	if (_rect) {
		delete _rect;
		_rect = 0;
	}
}

void 
CanvasFlag::set_text(string& a_text)
{
	delete_allocated_objects();
	
	_text = new Text(*this, 0.0, 0.0, Glib::ustring(a_text));
	_text->property_justification() = Gtk::JUSTIFY_CENTER;
	_text->property_fill_color_rgba() = _outline_color_rgba;
	double flagwidth  = _text->property_text_width()  + 10.0;
	double flagheight = _text->property_text_height() + 3.0;
	_text->property_x() = flagwidth / 2.0;
	_text->property_y() = flagheight / 2.0;
	_text->show();
	_line = new SimpleLine(*this, 0.0, 0.0, 0.0, _height);
	_line->property_color_rgba() = _outline_color_rgba;
	_rect = new SimpleRect(*this, 0.0, 0.0, flagwidth, flagheight);
	_rect->property_outline_color_rgba() = _outline_color_rgba;
	_rect->property_fill_color_rgba() = _fill_color_rgba;
	_text->lower_to_bottom();
	_text->raise(2);	
}

CanvasFlag::~CanvasFlag()
{
	delete_allocated_objects();
}
