/*
    Copyright (C) 2003 Paul Davis 

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

#ifndef __gtkmm2ext_fastmeter_h__
#define __gtkmm2ext_fastmeter_h__

#include <gtkmm/drawingarea.h>
#include <gdkmm/pixbuf.h>

namespace Gtkmm2ext {

class FastMeter : public Gtk::DrawingArea {
  public:
	enum Orientation { 
		Horizontal,
		Vertical
	};
	
	FastMeter (long hold_cnt, unsigned long width, Orientation, int len=0, int clrb0=0x00ff00, int clr1=0xffff00, int clr2=0xffaa00, int clr3=0xff0000);
	virtual ~FastMeter ();
	
	void set (float level);
	void clear ();

	float get_level() { return current_level; }
	float get_user_level() { return current_user_level; }
	float get_peak() { return current_peak; }

	long hold_count() { return hold_cnt; }
	void set_hold_count (long);
	
  protected:
	bool on_expose_event (GdkEventExpose*);
	void on_size_request (GtkRequisition*);
	void on_size_allocate (Gtk::Allocation&);

  private:  

	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	gint pixheight;
	gint pixwidth;
	static int _clr0, _clr1, _clr2, _clr3;

	Orientation orientation;
	GdkRectangle pixrect;
	GdkRectangle last_peak_rect;
	gint request_width;
	gint request_height;
	unsigned long hold_cnt;
	unsigned long hold_state;
	float current_level;
	float current_peak;
	float current_user_level;
	
	bool vertical_expose (GdkEventExpose*);
	bool horizontal_expose (GdkEventExpose*);
	void queue_vertical_redraw (const Glib::RefPtr<Gdk::Window>&, float);
	void queue_horizontal_redraw (const Glib::RefPtr<Gdk::Window>&, float);

	static Glib::RefPtr<Gdk::Pixbuf> request_vertical_meter(int w, int h);

	static Glib::RefPtr<Gdk::Pixbuf> *v_pixbuf_cache;
	static int min_v_pixbuf_size;
	static int max_v_pixbuf_size;

	static Glib::RefPtr<Gdk::Pixbuf> request_horizontal_meter(int w, int h);

	static Glib::RefPtr<Gdk::Pixbuf> *h_pixbuf_cache;
	static int min_h_pixbuf_size;
	static int max_h_pixbuf_size;
};


} /* namespace */

 #endif /* __gtkmm2ext_fastmeter_h__ */
