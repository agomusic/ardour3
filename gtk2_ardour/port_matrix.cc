/*
    Copyright (C) 2002-2009 Paul Davis 

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

#include <iostream>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>
#include "ardour/bundle.h"
#include "port_matrix.h"
#include "i18n.h"

/** PortMatrix constructor.
 *  @param session Our session.
 *  @param type Port type that we are handling.
 *  @param offer_inputs true to offer inputs, otherwise false.
 *  @param mask Mask of port groups to offer.
 */
PortMatrix::PortMatrix (ARDOUR::Session& session, ARDOUR::DataType type, bool offer_inputs, PortGroupList::Mask mask)
	: _offer_inputs (offer_inputs),
	  _port_group_list (session, type, offer_inputs, mask),
	  _type (type),
	  _body (this, offer_inputs ? PortMatrixBody::BOTTOM_AND_LEFT : PortMatrixBody::TOP_AND_RIGHT)
{
	/* checkbuttons for visibility of groups */
	Gtk::HBox* visibility_buttons = Gtk::manage (new Gtk::HBox);

	visibility_buttons->pack_start (*Gtk::manage (new Gtk::Label (_("Show:"))), Gtk::PACK_SHRINK);
	
	for (std::list<PortGroup*>::iterator i = _port_group_list.begin(); i != _port_group_list.end(); ++i) {
		_port_group_uis.push_back (new PortGroupUI (this, *i));
	}

	for (std::list<PortGroupUI*>::iterator i = _port_group_uis.begin(); i != _port_group_uis.end(); ++i) {
		visibility_buttons->pack_start ((*i)->visibility_checkbutton(), Gtk::PACK_SHRINK);
	}

	pack_start (*visibility_buttons, Gtk::PACK_SHRINK);
	pack_start (_hscroll, Gtk::PACK_SHRINK);
	Gtk::HBox* hbox = Gtk::manage (new Gtk::HBox);
	hbox->pack_start (_body);
	hbox->pack_start (_vscroll, Gtk::PACK_SHRINK);
	pack_start (*hbox);

	_hscroll.signal_value_changed().connect (sigc::mem_fun (*this, &PortMatrix::hscroll_changed));
	_vscroll.signal_value_changed().connect (sigc::mem_fun (*this, &PortMatrix::vscroll_changed));
	setup_scrollbars ();

	/* XXX hard-coded initial size suggestion */
	set_size_request (400, 200);
	show_all ();
}

PortMatrix::~PortMatrix ()
{
	for (std::list<PortGroupUI*>::iterator i = _port_group_uis.begin(); i != _port_group_uis.end(); ++i) {
		delete *i;
	}
}

void
PortMatrix::setup ()
{
	_port_group_list.refresh ();

	std::vector<boost::shared_ptr<ARDOUR::Bundle> > column;
	std::vector<boost::shared_ptr<ARDOUR::Bundle> > row;

	for (PortGroupList::iterator i = _port_group_list.begin (); i != _port_group_list.end (); ++i) {
		if ((*i)->visible) {
			
			std::copy ((*i)->bundles.begin(), (*i)->bundles.end(), std::back_inserter (column));
			
			/* make a bundle for the ports, if there are any */
			if (!(*i)->ports.empty()) {

				boost::shared_ptr<ARDOUR::Bundle> b (new ARDOUR::Bundle ("", _type, !_offer_inputs));
				
				std::string const pre = common_prefix ((*i)->ports);
				if (!pre.empty()) {
					b->set_name (pre.substr (0, pre.length() - 1));
				}

				for (uint32_t j = 0; j < (*i)->ports.size(); ++j) {
					std::string const p = (*i)->ports[j];
					b->add_channel (p.substr (pre.length()));
					b->set_port (j, p);
				}
					
				column.push_back (b);
			}
		}
	}

	row.push_back (_our_bundle);

	_body.setup (row, column);
	setup_scrollbars ();
	queue_draw ();
}

void
PortMatrix::set_offer_inputs (bool s)
{
	_offer_inputs = s;
	_port_group_list.set_offer_inputs (s);
	setup ();
}

void
PortMatrix::set_type (ARDOUR::DataType t)
{
	_type = t;
	_port_group_list.set_type (t);
	setup ();
}

void
PortMatrix::hscroll_changed ()
{
	_body.set_xoffset (_hscroll.get_adjustment()->get_value());
}

void
PortMatrix::vscroll_changed ()
{
	_body.set_yoffset (_vscroll.get_adjustment()->get_value());
}

void
PortMatrix::setup_scrollbars ()
{
	Gtk::Adjustment* a = _hscroll.get_adjustment ();
	a->set_lower (0);
	a->set_upper (_body.full_scroll_width());
	a->set_page_size (_body.alloc_scroll_width());
	a->set_step_increment (32);
	a->set_page_increment (128);

	a = _vscroll.get_adjustment ();
	a->set_lower (0);
	a->set_upper (_body.full_scroll_height());
	a->set_page_size (_body.alloc_scroll_height());
	a->set_step_increment (32);
	a->set_page_increment (128);
}

std::string
PortMatrix::common_prefix (std::vector<std::string> const & p) const
{
	/* common prefix before '/' ? */
	if (p[0].find_first_of ("/") != std::string::npos) {
		std::string const fp = p[0].substr (0, (p[0].find_first_of ("/") + 1));
		uint32_t j = 1;
		while (j < p.size()) {
			if (p[j].substr (0, fp.length()) != fp) {
				break;
			}
			++j;
		}
		
		if (j == p.size()) {
			return fp;
		}
	}
	
	/* or before ':' ? */
	if (p[0].find_first_of (":") != std::string::npos) {
		std::string const fp = p[0].substr (0, (p[0].find_first_of (":") + 1));
		uint32_t j = 1;
		while (j < p.size()) {
			if (p[j].substr (0, fp.length()) != fp) {
				break;
			}
			++j;
		}
		
		if (j == p.size()) {
			return fp;
		}
	}

	return "";
}

void
PortMatrix::disassociate_all ()
{
	for (PortGroupList::iterator i = _port_group_list.begin(); i != _port_group_list.end(); ++i) {
		
		for (std::vector<boost::shared_ptr<ARDOUR::Bundle> >::iterator j = (*i)->bundles.begin(); j != (*i)->bundles.end(); ++j) {

			for (uint32_t k = 0; k < (*j)->nchannels(); ++k) {

				for (uint32_t l = 0; l < _our_bundle->nchannels(); ++l) {

					set_state (
						_our_bundle, l, *j, k, false, 0
						);
				}
			}
		}
	}

	_body.rebuild_and_draw_grid ();
}
