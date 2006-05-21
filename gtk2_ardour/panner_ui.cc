/*
  Copyright (C) 2004 Paul Davis

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

#include <limits.h>

#include <ardour/io.h>
#include <ardour/dB.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/stop_signal.h>
#include <gtkmm2ext/barcontroller.h>
#include <midi++/manager.h>
#include <pbd/fastlog.h>

#include "ardour_ui.h"
#include "panner_ui.h"
#include "panner2d.h"
#include "utils.h"
#include "gui_thread.h"

#include <ardour/session.h>
#include <ardour/panner.h>
#include <ardour/route.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtkmm2ext;
using namespace Gtk;
using namespace sigc;


PannerUI::PannerUI (IO& io, Session& s)
	: _io (io),
	  _session (s),
	  hAdjustment(0.0, 0.0, 0.0),
	  vAdjustment(0.0, 0.0, 0.0),
	  panning_viewport(hAdjustment, vAdjustment),
	  panning_up_arrow (Gtk::ARROW_UP, Gtk::SHADOW_OUT),
	  panning_down_arrow (Gtk::ARROW_DOWN, Gtk::SHADOW_OUT),
	  panning_link_button (_("link")),
	  pan_automation_style_button (""),
	  pan_automation_state_button ("")
{
	ignore_toggle = false;
	pan_menu = 0;
	in_pan_update = false;

	pan_automation_style_button.set_name ("MixerAutomationModeButton");
	pan_automation_state_button.set_name ("MixerAutomationPlaybackButton");

	ARDOUR_UI::instance()->tooltips().set_tip (pan_automation_state_button, _("Pan automation mode"));
	ARDOUR_UI::instance()->tooltips().set_tip (pan_automation_style_button, _("Pan automation type"));

	set_size_request_to_display_given_text (pan_automation_state_button, X_("M"), 2, 2);
	set_size_request_to_display_given_text (pan_automation_style_button, X_("M"), 2, 2);

	pan_bar_packer.set_size_request (-1, 61);
	panning_viewport.set_size_request (61, 61);

	panning_viewport.set_name (X_("BaseFrame"));

	ARDOUR_UI::instance()->tooltips().set_tip (panning_link_button,
						   _("panning link control"));
	ARDOUR_UI::instance()->tooltips().set_tip (panning_link_direction_button,
						   _("panning link direction"));

	pan_automation_style_button.unset_flags (Gtk::CAN_FOCUS);
	pan_automation_state_button.unset_flags (Gtk::CAN_FOCUS);

	using namespace Menu_Helpers;
	pan_astate_menu.items().push_back (MenuElem (_("Manual"), 
						     bind (mem_fun (_io.panner(), &Panner::set_automation_state), (AutoState) Off)));
	pan_astate_menu.items().push_back (MenuElem (_("Play"),
						     bind (mem_fun (_io.panner(), &Panner::set_automation_state), (AutoState) Play)));
	pan_astate_menu.items().push_back (MenuElem (_("Write"),
						     bind (mem_fun (_io.panner(), &Panner::set_automation_state), (AutoState) Write)));
	pan_astate_menu.items().push_back (MenuElem (_("Touch"),
						     bind (mem_fun (_io.panner(), &Panner::set_automation_state), (AutoState) Touch)));

	pan_astyle_menu.items().push_back (MenuElem (_("Trim")));
	pan_astyle_menu.items().push_back (MenuElem (_("Abs")));

	pan_astate_menu.set_name ("ArdourContextMenu");
	pan_astyle_menu.set_name ("ArdourContextMenu");

	pan_automation_style_button.signal_button_press_event().connect (mem_fun(*this, &PannerUI::pan_automation_style_button_event), false);
	pan_automation_state_button.signal_button_press_event().connect (mem_fun(*this, &PannerUI::pan_automation_state_button_event), false);

	panning_link_button.set_name (X_("PanningLinkButton"));
	panning_link_direction_button.set_name (X_("PanningLinkDirectionButton"));

	/* the pixmap will be reset at some point, but the key thing is that
	   we need a pixmap in the button just to get started.
	*/

	Gtk::HBox* pan_button_hbox = manage (new Gtk::HBox());

	panning_link_box.pack_start (panning_link_button, true, true);
	panning_link_box.pack_start (panning_link_direction_button, true, true);
	pan_button_hbox->pack_start (panning_link_box, true, true);
	pan_button_hbox->pack_start (pan_automation_state_button, true, true);

	panning_link_direction_button.add (*(manage (new Image (get_xpm("forwardblarrow.xpm")))));

	panning_link_direction_button.signal_clicked().connect
		(mem_fun(*this, &PannerUI::panning_link_direction_clicked));

	panning_link_button.signal_button_press_event().connect
		(mem_fun(*this, &PannerUI::panning_link_button_press));
	panning_link_button.signal_button_release_event().connect
		(mem_fun(*this, &PannerUI::panning_link_button_release));

	panning_up.set_border_width (3);
	panning_down.set_border_width (3);
	panning_up.add (panning_up_arrow);
	panning_down.add (panning_down_arrow);
	panning_up.set_name (X_("PanScrollerBase"));
	panning_down.set_name (X_("PanScrollerBase"));
	panning_up_arrow.set_name (X_("PanScrollerArrow"));
	panning_down_arrow.set_name (X_("PanScrollerArrow"));

	pan_vbox.set_spacing (4);
	pan_vbox.pack_start (panning_viewport, Gtk::PACK_SHRINK);
	pan_vbox.pack_start (*pan_button_hbox, Gtk::PACK_SHRINK);

	pack_start (pan_vbox, true, false);

	panner = 0;

	_io.panner().Changed.connect (mem_fun(*this, &PannerUI::panner_changed));
	_io.panner().LinkStateChanged.connect (mem_fun(*this, &PannerUI::update_pan_linkage));
	_io.panner().StateChanged.connect (mem_fun(*this, &PannerUI::update_pan_state));

	pan_changed (0);
	update_pan_sensitive ();
	update_pan_linkage ();
	pan_automation_state_changed ();
}

gint
PannerUI::panning_link_button_press (GdkEventButton* ev)
{
	return stop_signal (panning_link_button, "button-press-event");
}

gint
PannerUI::panning_link_button_release (GdkEventButton* ev)
{
	if (!ignore_toggle) {
		_io.panner().set_linked (!_io.panner().linked());
	}
	return TRUE;
}

void
PannerUI::panning_link_direction_clicked()
{
	switch (_io.panner().link_direction()) {
	case Panner::SameDirection:
		_io.panner().set_link_direction (Panner::OppositeDirection);
		break;
	default:
		_io.panner().set_link_direction (Panner::SameDirection);
		break;
	}
}

void
PannerUI::update_pan_linkage ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &PannerUI::update_pan_linkage));
	
	bool x = _io.panner().linked();
	bool bx = panning_link_button.get_active();
	
	if (x != bx) {
		
		ignore_toggle = true;
		panning_link_button.set_active (x);
		ignore_toggle = false;
	}

	panning_link_direction_button.set_sensitive (x);

	switch (_io.panner().link_direction()) {
	case Panner::SameDirection:
	        panning_link_direction_button.set_image (*(manage (new Image (get_xpm ("forwardblarrow.xpm")))));
		break;
	default:
	        panning_link_direction_button.set_image (*(manage (new Image (get_xpm("revdblarrow.xpm")))));
		break;
	}
}

void
PannerUI::set_width (Width w)
{
	switch (w) {
	case Wide:
		panning_viewport.set_size_request (61, 61);
		if (panner) {
			panner->set_size_request (61, 61);
		}
		for (vector<BarController*>::iterator i = pan_bars.begin(); i != pan_bars.end(); ++i) {
				(*i)->set_size_request (61, 15);
		}
		panning_link_button.set_label (_("link"));
		break;
	case Narrow:
		panning_viewport.set_size_request (31, 61);
		if (panner) {
			panner->set_size_request (31, 61);
		}
		for (vector<BarController*>::iterator i = pan_bars.begin(); i != pan_bars.end(); ++i) {
				(*i)->set_size_request (31, 15);
		}
		panning_link_button.set_label (_("L"));
		break;
	}

	_width = w;
}


PannerUI::~PannerUI ()
{
	for (vector<Adjustment*>::iterator i = pan_adjustments.begin(); i != pan_adjustments.end(); ++i) {
		delete (*i);
	}
	
	for (vector<BarController*>::iterator i = pan_bars.begin(); i != pan_bars.end(); ++i) {
		delete (*i);
	}

	if (panner) {
		delete panner;
	}

	if (pan_menu) {
		delete pan_menu;
	}

}


void
PannerUI::panner_changed ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &PannerUI::panner_changed));
	setup_pan ();
}

void
PannerUI::update_pan_state ()
{
	/* currently nothing to do */
	// ENSURE_GUI_THREAD (mem_fun(*this, &PannerUI::update_panner_state));
}

void
PannerUI::setup_pan ()
{
	uint32_t nouts = _io.n_outputs ();

	if (nouts == 0 || nouts == 1) {

		while (!pan_adjustments.empty()) {
			delete pan_bars.back();
			pan_bars.pop_back ();
			delete pan_adjustments.back();
			pan_adjustments.pop_back ();
		}

	} else if (nouts == 2) {

		vector<Adjustment*>::size_type asz;
		uint32_t npans = _io.panner().size();

		while (!pan_adjustments.empty()) {
			delete pan_bars.back();
			pan_bars.pop_back ();
			delete pan_adjustments.back();
			pan_adjustments.pop_back ();
		}

		while ((asz = pan_adjustments.size()) < npans) {

			float x;
			BarController* bc;

			/* initialize adjustment with current value of panner */

			_io.panner()[asz]->get_position (x);

			pan_adjustments.push_back (new Adjustment (x, 0, 1.0, 0.05, 0.1));
			pan_adjustments.back()->signal_value_changed().connect (bind (mem_fun(*this, &PannerUI::pan_adjustment_changed), (uint32_t) asz));

			_io.panner()[asz]->Changed.connect (bind (mem_fun(*this, &PannerUI::pan_value_changed), (uint32_t) asz));

			bc = new BarController (*pan_adjustments[asz], 
						&_io.panner()[asz]->midi_control(),
						bind (mem_fun(*this, &PannerUI::pan_printer), pan_adjustments[asz]));
			
			if (_session.midi_port()) {
				_io.panner()[asz]->reset_midi_control (_session.midi_port(), true);
			}
			
			bc->set_name ("PanSlider");
			bc->set_shadow_type (Gtk::SHADOW_NONE);
			bc->set_style (BarController::Line);

			bc->StartGesture.connect (bind (mem_fun (_io, &IO::start_pan_touch), (uint32_t) asz));
			bc->StopGesture.connect (bind (mem_fun (_io, &IO::end_pan_touch), (uint32_t) asz));

			char buf[64];
#ifdef __APPLE__
			snprintf (buf, sizeof (buf), _("panner for channel %lu"), asz + 1);
#else
			snprintf (buf, sizeof (buf), _("panner for channel %u"), asz + 1);
#endif
			ARDOUR_UI::instance()->tooltips().set_tip (bc->event_widget(), buf);

			bc->event_widget().signal_button_release_event().connect
				(bind (mem_fun(*this, &PannerUI::pan_button_event), (uint32_t) asz));

			pan_bars.push_back (bc);
			switch (_width) {
			case Wide:
				pan_bars.back()->set_size_request (61, 15);
				break;
			case Narrow:
				pan_bars.back()->set_size_request (31, 15);
				break;
			}

			pan_bar_packer.pack_start (*pan_bars.back(), false, false);
		}

		/* now that we actually have the pan bars,
		   set their sensitivity based on current
		   automation state.
		*/

		update_pan_sensitive ();

		panning_viewport.remove ();
		panning_viewport.add (pan_bar_packer);
		panning_viewport.show_all ();

	} else {

		int w = 0;

		switch (_width) {
		case Wide:
			w = 61;
			break;
		case Narrow:
			w = 31;
			break;
		}

		if (panner == 0) {
			panner = new Panner2d (_io.panner(), w, 61);
			panner->set_name ("MixerPanZone");
			panner->show ();
		}
		
		update_pan_sensitive ();
		panner->reset (_io.n_inputs());
		panner->set_size_request (w, 61);

		/* and finally, add it to the panner frame */

		panning_viewport.remove ();
		panning_viewport.add (*panner);
		panning_viewport.show_all ();
	}
}

gint
PannerUI::pan_button_event (GdkEventButton* ev, uint32_t which)
{
	switch (ev->button) {
	case 3:
		if (pan_menu == 0) {
			pan_menu = manage (new Menu);
			pan_menu->set_name ("ArdourContextMenu");
		}
		build_pan_menu (which);
		pan_menu->popup (1, ev->time);
		return TRUE;
		break;
	default:
		return FALSE;
	}

	return FALSE; // what's wrong with gcc?
}

void
PannerUI::build_pan_menu (uint32_t which)
{
	using namespace Menu_Helpers;
	MenuList& items (pan_menu->items());

	items.clear ();

	items.push_back (CheckMenuElem (_("Mute")));
	
	/* set state first, connect second */

	(dynamic_cast<CheckMenuItem*> (&items.back()))->set_active (_io.panner()[which]->muted());
	(dynamic_cast<CheckMenuItem*> (&items.back()))->signal_toggled().connect
		(bind (mem_fun(*this, &PannerUI::pan_mute), which));

	items.push_back (CheckMenuElem (_("Bypass"), mem_fun(*this, &PannerUI::pan_bypass_toggle)));
	bypass_menu_item = static_cast<CheckMenuItem*> (&items.back());

	/* set state first, connect second */

	bypass_menu_item->set_active (_io.panner().bypassed());
	bypass_menu_item->signal_toggled().connect (mem_fun(*this, &PannerUI::pan_bypass_toggle));

	items.push_back (MenuElem (_("Reset"), mem_fun(*this, &PannerUI::pan_reset)));
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Reset all")));
}

void
PannerUI::pan_mute (uint32_t which)
{
	StreamPanner* sp = _io.panner()[which];
	sp->set_muted (!sp->muted());
}

void
PannerUI::pan_bypass_toggle ()
{
	if (bypass_menu_item && (_io.panner().bypassed() != bypass_menu_item->get_active())) {
		_io.panner().set_bypassed (!_io.panner().bypassed());
	}
}

void
PannerUI::pan_reset ()
{
}

void
PannerUI::effective_pan_display ()
{
	if (_io.panner().empty()) {
		return;
	}

	switch (_io.n_outputs()) {
	case 0: 
	case 1:
		/* relax */
		break;

	case 2:
		update_pan_bars (true);
		break;

	default:
                //panner->move_puck (pan_value (v, right), 0.5);
		break;
	}
}

void
PannerUI::pan_changed (void *src)
{
	if (src == this) {
		return;
	}

	switch (_io.panner().size()) {
	case 0:
		panning_link_box.set_sensitive (false);
		return;
	case 1:
		panning_link_box.set_sensitive (false);
		break;
	default:
		panning_link_box.set_sensitive (true);
	}

	uint32_t nouts = _io.n_outputs();

	switch (nouts) {
	case 0:
	case 1:
		/* relax */
		break;

	case 2:
		update_pan_bars (false);
		break;

	default:
		// panner->move_puck (pan_value (pans[0], pans[1]), 0.5);
		break;
	}
}

void
PannerUI::pan_adjustment_changed (uint32_t which)
{
	if (!in_pan_update && which < _io.panner().size()) {

		float xpos;
		float val = pan_adjustments[which]->get_value ();
		_io.panner()[which]->get_position (xpos);

		/* add a kinda-sorta detent for the middle */
		
		if (val != 0.5 && Panner::equivalent (val, 0.5)) {
			/* this is going to be reentrant, so just 
			   return after it.
			*/

			in_pan_update = true;
			pan_adjustments[which]->set_value (0.5);
			in_pan_update = false;
			return;
		}
		
		if (!Panner::equivalent (val, xpos)) {

			_io.panner()[which]->set_position (val);
			/* XXX 
			   the panner objects have no access to the session,
			   so do this here. ick.
			*/
			_session.set_dirty();
		}
	}
}

void
PannerUI::pan_value_changed (uint32_t which)
{
	ENSURE_GUI_THREAD (bind (mem_fun(*this, &PannerUI::pan_value_changed), which));
							   
	if (_io.n_outputs() > 1 && which < _io.panner().size()) {
		float xpos;
		float val = pan_adjustments[which]->get_value ();

		_io.panner()[which]->get_position (xpos);

		if (!Panner::equivalent (val, xpos)) {
			in_pan_update = true;
			pan_adjustments[which]->set_value (xpos);
			in_pan_update = false;
		}
	}
}	

void
PannerUI::update_pan_bars (bool only_if_aplay)
{
	uint32_t n;
	vector<Adjustment*>::iterator i;

	in_pan_update = true;

	/* this runs during automation playback, and moves the bar controllers
	   and/or pucks around.
	*/

	for (i = pan_adjustments.begin(), n = 0; i != pan_adjustments.end(); ++i, ++n) {
		float xpos, val;

		if (only_if_aplay) {
			AutomationList& alist (_io.panner()[n]->automation());
			
			if (!alist.automation_playback()) {
				continue;
			}
		}

		_io.panner()[n]->get_effective_position (xpos);
		val = (*i)->get_value ();
		
		if (!Panner::equivalent (val, xpos)) {
			(*i)->set_value (xpos);
		}
	}

	in_pan_update = false;
}

void
PannerUI::pan_printer (char *buf, uint32_t len, Adjustment* adj)
{
	float val = adj->get_value();

	if (val == 0.0f) {
		snprintf (buf, len, X_("L"));
	} else if (val == 1.0f) {
		snprintf (buf, len, X_("R"));
	} else if (Panner::equivalent (val, 0.5f)) {
		snprintf (buf, len, X_("C"));
	} else {
		/* don't print anything */
		buf[0] = '\0';
	}
}

void
PannerUI::update_pan_sensitive ()
{
	bool sensitive = !(_io.panner().automation_state() & Play);

	switch (_io.n_outputs()) {
	case 0:
	case 1:
		break;
	case 2:
		for (vector<BarController*>::iterator i = pan_bars.begin(); i != pan_bars.end(); ++i) {
			(*i)->set_sensitive (sensitive);
		}
		break;
	default:
		if (panner) {
			panner->set_sensitive (sensitive);
		}
		break;
	}
}

gint
PannerUI::pan_automation_state_button_event (GdkEventButton *ev)
{
	using namespace Menu_Helpers;

	if (ev->type == GDK_BUTTON_RELEASE) {
		return TRUE;
	}

	switch (ev->button) {
	case 1:
		pan_astate_menu.popup (1, ev->time);
		break;
	default:
		break;
	}

	return TRUE;
}

gint
PannerUI::pan_automation_style_button_event (GdkEventButton *ev)
{
	if (ev->type == GDK_BUTTON_RELEASE) {
		return TRUE;
	}

	switch (ev->button) {
	case 1:
		pan_astyle_menu.popup (1, ev->time);
		break;
	default:
		break;
	}
	return TRUE;
}

void
PannerUI::pan_automation_style_changed ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &PannerUI::pan_automation_style_changed));
	
	switch (_width) {
	case Wide:
	        pan_automation_style_button.set_label (astyle_string(_io.panner().automation_style()));
		break;
	case Narrow:
	  	pan_automation_style_button.set_label (short_astyle_string(_io.panner().automation_style()));
		break;
	}
}

void
PannerUI::pan_automation_state_changed ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &PannerUI::pan_automation_state_changed));
	
	bool x;

	switch (_width) {
	case Wide:
	  pan_automation_state_button.set_label (astate_string(_io.panner().automation_state()));
		break;
	case Narrow:
	  pan_automation_state_button.set_label (short_astate_string(_io.panner().automation_state()));
		break;
	}

	/* when creating a new session, we get to create busses (and
	   sometimes tracks) with no outputs by the time they get
	   here.
	*/

	if (_io.panner().empty()) {
		return;
	}

	x = (_io.panner().front()->automation().automation_state() != Off);

	if (pan_automation_state_button.get_active() != x) {
	ignore_toggle = true;
		pan_automation_state_button.set_active (x);
		ignore_toggle = false;
	}

	update_pan_sensitive ();

	/* start watching automation so that things move */

	pan_watching.disconnect();

	if (x) {
	  pan_watching = ARDOUR_UI::RapidScreenUpdate.connect (mem_fun (*this, &PannerUI::effective_pan_display));
	}
}

string
PannerUI::astate_string (AutoState state)
{
	return _astate_string (state, false);
}

string
PannerUI::short_astate_string (AutoState state)
{
	return _astate_string (state, true);
}

string
PannerUI::_astate_string (AutoState state, bool shrt)
{
	string sstr;

	switch (state) {
	case Off:
		sstr = (shrt ? "M" : _("M"));
		break;
	case Play:
		sstr = (shrt ? "P" : _("P"));
		break;
	case Touch:
		sstr = (shrt ? "T" : _("T"));
		break;
	case Write:
		sstr = (shrt ? "W" : _("W"));
		break;
	}

	return sstr;
}

string
PannerUI::astyle_string (AutoStyle style)
{
	return _astyle_string (style, false);
}

string
PannerUI::short_astyle_string (AutoStyle style)
{
	return _astyle_string (style, true);
}

string
PannerUI::_astyle_string (AutoStyle style, bool shrt)
{
	if (style & Trim) {
		return _("Trim");
	} else {
	        /* XXX it might different in different languages */

		return (shrt ? _("Abs") : _("Abs"));
	}
}
