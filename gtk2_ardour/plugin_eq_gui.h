/*
    Copyright (C) 2008 Paul Davis
    Author: Sampo Savolainen

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

#ifndef __ardour_plugin_eq_gui_h
#define __ardour_plugin_eq_gui_h

#include <ardour/buffer_set.h>
#include <ardour/plugin_insert.h>
#include <ardour/plugin.h>

#include <gtkmm/table.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

class FFT;

class PluginEqGui : public Gtk::Table
{
	public:
		PluginEqGui(boost::shared_ptr<ARDOUR::PluginInsert>);
		~PluginEqGui();
		
	

	private:
		// Setup
		void set_buffer_size(uint32_t);
		void change_dB_scale();

		// Analysis
		void run_analysis();

		// Drawing
		virtual void on_hide();
		virtual void on_show();

		void stop_updating();
		void start_updating();

		void resize_analysis_area(Gtk::Allocation&);
		void redraw_analysis_area();

		void draw_analysis_scales(cairo_t *);
		bool expose_analysis_area(GdkEventExpose *);

		void draw_scales_power(Gtk::Widget *, cairo_t *);
		void plot_amplitude(Gtk::Widget *,cairo_t *);

		void draw_scales_phase(Gtk::Widget *,cairo_t *);
		void plot_phase(Gtk::Widget *,cairo_t *);

		// Helpers
		bool timeout_callback();
		void redraw_scales();


		// Fields:

		// analysis parameters
		float _samplerate;

		float _min_dB;
		float _max_dB;
		float _step_dB;


		float _log_coeff;
		float _log_max;

		nframes_t _buffer_size;

		// buffers		
		ARDOUR::BufferSet _bufferset;


		// dimensions
		float _analysis_width;
		float _analysis_height;

		// My objects
		FFT *_impulse_fft;
		boost::shared_ptr<ARDOUR::Plugin> _plugin;

		// gui objects
		Gtk::DrawingArea *_analysis_area;
		cairo_surface_t *_analysis_scale_surface;


		// dB scale selection:
		class dBSelectionColumns : public Gtk::TreeModel::ColumnRecord
		{
		  public:
			dBSelectionColumns()
				{ add(dBMin); add(dBMax); add(dBStep); add(name); }

			Gtk::TreeModelColumn<float> dBMin;
			Gtk::TreeModelColumn<float> dBMax;
			Gtk::TreeModelColumn<float> dBStep;
			Gtk::TreeModelColumn<Glib::ustring> name;
		};

		dBSelectionColumns dBColumns;

		Gtk::ComboBox *dBScaleCombo;
		Glib::RefPtr<Gtk::ListStore> dBScaleModel;

		Gtk::CheckButton *_phase_button;

		// signals and connections
		sigc::connection _update_connection;
		sigc::connection _window_unmap_connection;
		sigc::connection _window_map_connection;
};

#endif

