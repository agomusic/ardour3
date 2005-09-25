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

    $Id$
*/

#include <string>

#include <pbd/error.h>

#include <gtkmm2ext/utils.h>

#include <ardour/session.h>
#include <ardour/utils.h>

#include "ardour_ui.h"
#include "public_editor.h"
#include "imageframe_time_axis.h"
#include "canvas-simplerect.h"
#include "selection.h"
#include "imageframe_time_axis_view.h"
#include "marker_time_axis_view.h"
#include "imageframe_view.h"
#include "marker_time_axis.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace sigc;
using namespace Gtk;

//---------------------------------------------------------------------------------------//
// Constructor / Desctructor

/**
 * Constructs a new MarkerTimeAxis
 *
 * @param ed the PublicEditor
 * @param sess the current session
 * @param canvas the parent canvas item
 * @param name the name/id of this time axis
 * @param tav the associated track view that this MarkerTimeAxis is marking up
 */
MarkerTimeAxis::MarkerTimeAxis (PublicEditor& ed, ARDOUR::Session& sess, Widget *canvas, std::string name, TimeAxisView* tav)
	: AxisView(sess),
	  VisualTimeAxis(name, ed, sess, canvas)
{
	/* the TimeAxisView these markers are associated with */
	marked_time_axis = tav ;
	
	_color = unique_random_color() ;
	time_axis_name = name ;

	selection_group = gtk_canvas_item_new (GTK_CANVAS_GROUP(canvas_display), gtk_canvas_group_get_type (), 0) ;
	gtk_canvas_item_hide(selection_group) ;

	// intialize our data items
	name_prompter = 0 ;
	marker_menu = 0 ;

	y_position = -1 ;

	/* create our new marker time axis strip view */
	view = new MarkerTimeAxisView(*this) ;

	// set the initial time axis text label
	label_view() ;
		
	// set the initial height of this time axis
	set_height(Small) ;
}

/**
 * Destructor
 * Responsible for destroying any marker items upon this time axis
 */
MarkerTimeAxis::~MarkerTimeAxis()
{
	 GoingAway() ; /* EMIT_SIGNAL */

	for (list<SelectionRect*>::iterator i = free_selection_rects.begin(); i != free_selection_rects.end(); ++i)
	{
		gtk_object_destroy (GTK_OBJECT((*i)->rect));
		gtk_object_destroy (GTK_OBJECT((*i)->start_trim));
		gtk_object_destroy (GTK_OBJECT((*i)->end_trim));
	}

	for (list<SelectionRect*>::iterator i = used_selection_rects.begin(); i != used_selection_rects.end(); ++i)
	{
		gtk_object_destroy (GTK_OBJECT((*i)->rect));
		gtk_object_destroy (GTK_OBJECT((*i)->start_trim));
		gtk_object_destroy (GTK_OBJECT((*i)->end_trim));
	}
	
	if(selection_group)
	{
		gtk_object_destroy(GTK_OBJECT (selection_group)) ;
		selection_group = 0 ;
	}

	// destroy the view helper
	// this handles removing and destroying individual marker items
	if(view)
	{
		delete view ;
		view = 0 ;
	}
}


//---------------------------------------------------------------------------------------//
// ui methods & data
	
/**
 * Sets the height of this TrackView to one of the defined TrackHeights
 *
 * @param h the TrackHeight value to set
 */	
void
MarkerTimeAxis::set_height (TrackHeight h)
{
	VisualTimeAxis::set_height(h) ;
	
	// tell out view helper of the change too
	if (view != 0)
	{
		view->set_height((double) height) ;
	}
	
	// tell those interested that we have had our height changed
	 gui_changed("track_height",(void*)0) ; /* EMIT_SIGNAL */
}

/**
 * Sets the number of samples per unit that are used.
 * This is used to determine the sizes of items upon this time axis
 *
 * @param spu the number of samples per unit
 */
void
MarkerTimeAxis::set_samples_per_unit(double spu)
{
	TimeAxisView::set_samples_per_unit (editor.get_current_zoom());

	if (view) {
		view->set_samples_per_unit(spu) ;
	}
}

/**
 * Show the popup edit menu
 *
 * @param button the mouse button pressed
 * @param time when to show the popup
 * @param clicked_mv the MarkerView that the event ocured upon, or 0 if none
 * @param with_item true if an item has been selected upon the time axis, used to set context menu
 */
void
MarkerTimeAxis::popup_marker_time_axis_edit_menu(int button, int32_t time, MarkerView* clicked_mv, bool with_item)
{
	if (!marker_menu)
	{
		build_marker_menu() ;
	}

	if (with_item)
	{
		marker_item_menu->set_sensitive(true) ;
	}
	else
	{
		marker_item_menu->set_sensitive(false) ;
	}
	
	marker_menu->popup(button,time) ;
}


/**
 * convenience method to select a new track color and apply it to the view and view items
 *
 */
void
MarkerTimeAxis::select_track_color()
{
	if(VisualTimeAxis::choose_time_axis_color())
	{
		if(view)
		{
			view->apply_color(_color) ;
		}
	}
}

/**
 * Handles the building of the popup menu
 */
void
MarkerTimeAxis::build_display_menu()
{
	using namespace Menu_Helpers;

	/* get the size menu ready */
	build_size_menu() ;

	/* prepare it */
	TimeAxisView::build_display_menu();

	/* now fill it with our stuff */
	MenuList& items = display_menu->items();

	items.push_back(MenuElem (_("Rename"), slot (*this, &VisualTimeAxis::start_time_axis_rename)));

	items.push_back(SeparatorElem()) ;
	items.push_back(MenuElem (_("Height"), *size_menu));
	items.push_back(MenuElem (_("Color"), slot (*this, &MarkerTimeAxis::select_track_color)));
	items.push_back(SeparatorElem()) ;
	
	items.push_back(MenuElem (_("Remove"), bind(slot(*this, &MarkerTimeAxis::remove_this_time_axis), (void*)this)));
}

/**
 * handles the building of the MarkerView sub menu
 */
void
MarkerTimeAxis::build_marker_menu()
{
	using namespace Menu_Helpers;

	marker_menu = manage(new Menu) ;
	marker_menu->set_name ("ArdourContextMenu");
	MenuList& items = marker_menu->items();
	
	marker_item_menu = manage(new Menu) ;
	marker_item_menu->set_name ("ArdourContextMenu");
	MenuList& marker_sub_items = marker_item_menu->items() ;

	/* duration menu */
	Menu* duration_menu = manage(new Menu) ;
	duration_menu->set_name ("ArdourContextMenu");
	MenuList& duration_items = duration_menu->items() ;
	
	if(view)
	{
		duration_items.push_back(MenuElem (_("1 seconds"), bind (slot (view, &MarkerTimeAxisView::set_marker_duration_sec), 1.0))) ;
		duration_items.push_back(MenuElem (_("1.5 seconds"), bind (slot (view, &MarkerTimeAxisView::set_marker_duration_sec), 1.5))) ;
		duration_items.push_back(MenuElem (_("2 seconds"), bind (slot (view, &MarkerTimeAxisView::set_marker_duration_sec), 2.0))) ;
		duration_items.push_back(MenuElem (_("2.5 seconds"), bind (slot (view, &MarkerTimeAxisView::set_marker_duration_sec), 2.5))) ;
		duration_items.push_back(MenuElem (_("3 seconds"), bind (slot (view, &MarkerTimeAxisView::set_marker_duration_sec), 3.0))) ;
	}
	//duration_items.push_back(SeparatorElem()) ;
	//duration_items.push_back(MenuElem (_("custom"), slot (*this, &ImageFrameTimeAxis::set_marker_duration_custom))) ;

	marker_sub_items.push_back(MenuElem(_("Duration (sec)"), *duration_menu)) ;

	marker_sub_items.push_back(SeparatorElem()) ;
	marker_sub_items.push_back(MenuElem (_("Remove Marker"), bind(slot(view, &MarkerTimeAxisView::remove_selected_marker_view),(void*)this))) ;
	
	items.push_back(MenuElem(_("Marker"), *marker_item_menu)) ;
	items.push_back(MenuElem (_("Rename Track"), slot (*this,&MarkerTimeAxis::start_time_axis_rename))) ;

	marker_menu->show_all() ;
}



/**
 * Returns the view helper of this TimeAxis
 *
 * @return the view helper of this TimeAxis
 */
MarkerTimeAxisView*
MarkerTimeAxis::get_view()
{
	return(view) ;
}

/**
 * Returns the TimeAxisView that this markerTimeAxis is marking up
 *
 * @return the TimeAXisView that this MarkerTimeAxis is marking
 */
TimeAxisView*
MarkerTimeAxis::get_marked_time_axis()
{
	return(marked_time_axis) ;
}




/**
 * Handle the closing of the renaming dialog during the rename of this item
 */
void
MarkerTimeAxis::finish_route_rename()
{
	name_prompter->hide_all ();
	ARDOUR_UI::instance()->allow_focus (false);

	if (name_prompter->status == Gtkmm2ext::Prompter::cancelled) {
		return;
	}
	
	string result;
	name_prompter->get_result(result);
	time_axis_name = result ;
	editor.route_name_changed(this) ;
	label_view() ;
	delete name_prompter ;
	name_prompter = 0 ;
}







