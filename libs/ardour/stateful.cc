/*
    Copyright (C) 2000-2001 Paul Davis 

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

#include <cstdio>
#include <unistd.h>

#include <ardour/stateful.h>
#include <ardour/utils.h>
#include <pbd/xml++.h>

#include "i18n.h"

Stateful::Stateful ()
{
	_extra_xml = 0;
	_instant_xml = 0;
}

Stateful::~Stateful ()
{
	// Do not delete _extra_xml.  The use of add_child_nocopy() 
	// means it needs to live on indefinately.
	delete _instant_xml;
}

void
Stateful::add_extra_xml (XMLNode& node)
{
	if (_extra_xml == 0) {
		_extra_xml = new XMLNode ("extra");
	}

	_extra_xml->remove_nodes (node.name());
	_extra_xml->add_child_nocopy (node);
}

XMLNode *
Stateful::extra_xml (const string& str)
{
	if (_extra_xml == 0) {
		return 0;
	}

	const XMLNodeList& nlist = _extra_xml->children();
	XMLNodeConstIterator i;

	for (i = nlist.begin(); i != nlist.end(); ++i) {
		if ((*i)->name() == str) {
			return (*i);
		}
	}

	return 0;
}

void
Stateful::add_instant_xml (XMLNode& node, const string& dir)
{
	if (_instant_xml == 0) {
		_instant_xml = new XMLNode ("instant");
	}

	_instant_xml->remove_nodes_and_delete (node.name());
	_instant_xml->add_child_copy (node);
	
	XMLTree tree;
	tree.set_filename(dir+"/instant.xml");

	/* Important: the destructor for an XMLTree deletes
	   all of its nodes, starting at _root. We therefore
	   cannot simply hand it our persistent _instant_xml 
	   node as its _root, because we will lose it whenever
	   the Tree goes out of scope.

	   So instead, copy the _instant_xml node (which does 
	   a deep copy), and hand that to the tree.
	*/

	XMLNode* copy = new XMLNode (*_instant_xml);
	tree.set_root (copy);

	if (!tree.write()) {
		error << compose(_("Error: could not write %1"), dir+"/instant.xml") << endmsg;
	}
}

XMLNode *
Stateful::instant_xml (const string& str, const string& dir)
{
	if (_instant_xml == 0) {
		string instant_file = dir + "/instant.xml";
		if (access(instant_file.c_str(), F_OK) == 0) {
			XMLTree tree;
			if (tree.read(dir+"/instant.xml")) {
				_instant_xml = new XMLNode(*(tree.root()));
			} else {
				warning << compose(_("Could not understand XML file %1"), instant_file) << endmsg;
				return 0;
			}
		} else {
			return 0;
		}
	}

	const XMLNodeList& nlist = _instant_xml->children();
	XMLNodeConstIterator i;

	for (i = nlist.begin(); i != nlist.end(); ++i) {
		if ((*i)->name() == str) {
			return (*i);
		}
	}

	return 0;
}

