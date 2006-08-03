/*
    Copyright (C) 2000-2006 Paul Davis 

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

#include <sys/types.h>
#include <cstdio>
#include <lrdf.h>
#include <dlfcn.h>

#ifdef VST_SUPPORT
#include <fst.h>
#include <pbd/basename.h>
#include <string.h>
#endif // VST_SUPPORT

#include <pbd/pathscanner.h>

#include <ardour/ladspa.h>
#include <ardour/session.h>
#include <ardour/plugin_manager.h>
#include <ardour/plugin.h>
#include <ardour/ladspa_plugin.h>
#include <ardour/vst_plugin.h>
#include <ardour/audio_unit.h>

#include <pbd/error.h>
#include <pbd/stl_delete.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;

PluginManager* PluginManager::_manager = 0;

PluginManager::PluginManager ()
{
	char* s;
	string lrdf_path;

	if ((s = getenv ("LADSPA_RDF_PATH"))){
		lrdf_path = s;
	}

	if (lrdf_path.length() == 0) {
		lrdf_path = "/usr/local/share/ladspa/rdf:/usr/share/ladspa/rdf";
	}

	add_lrdf_data(lrdf_path);
	add_ladspa_presets();
#ifdef VST_SUPPORT
	if (Config->get_use_vst()) {
		add_vst_presets();
	}
#endif /* VST_SUPPORT */

	if ((s = getenv ("LADSPA_PATH"))) {
		ladspa_path = s;
	}

	if ((s = getenv ("VST_PATH"))) {
		vst_path = s;
	} else if ((s = getenv ("VST_PLUGINS"))) {
		vst_path = s;
	}

	refresh ();

	if (_manager == 0) {
		_manager = this;
	}
}

void
PluginManager::refresh ()
{
	ladspa_refresh ();
#ifdef VST_SUPPORT
	if (Config->get_use_vst()) {
		vst_refresh ();
	}
#endif // VST_SUPPORT
}

void
PluginManager::ladspa_refresh ()
{
	_ladspa_plugin_info.clear ();

	if (ladspa_path.length() == 0) {
		ladspa_path = "/usr/local/lib/ladspa:/usr/lib/ladspa";
	}

	ladspa_discover_from_path (ladspa_path);
}

int
PluginManager::add_ladspa_directory (string path)
{
	if (ladspa_discover_from_path (path) == 0) {
		ladspa_path += ':';
		ladspa_path += path;
		return 0;
	} 
	return -1;
}

static bool ladspa_filter (const string& str, void *arg)
{
	/* Not a dotfile, has a prefix before a period, suffix is "so" */
	
	return str[0] != '.' && (str.length() > 3 && str.find (".so") == (str.length() - 3));
}

int
PluginManager::ladspa_discover_from_path (string path)
{
	PathScanner scanner;
	vector<string *> *plugin_objects;
	vector<string *>::iterator x;
	int ret = 0;

	plugin_objects = scanner (ladspa_path, ladspa_filter, 0, true, true);

	if (plugin_objects) {
		for (x = plugin_objects->begin(); x != plugin_objects->end (); ++x) {
			ladspa_discover (**x);
		}
	}

	vector_delete (plugin_objects);
	return ret;
}

static bool rdf_filter (const string &str, void *arg)
{
	return str[0] != '.' && 
		   ((str.find(".rdf")  == (str.length() - 4)) ||
            (str.find(".rdfs") == (str.length() - 5)) ||
		    (str.find(".n3")   == (str.length() - 3)));
}

void
PluginManager::add_ladspa_presets()
{
	add_presets ("ladspa");
}

void
PluginManager::add_vst_presets()
{
	add_presets ("vst");
}
void
PluginManager::add_presets(string domain)
{

	PathScanner scanner;
	vector<string *> *presets;
	vector<string *>::iterator x;

	char* envvar;
	if ((envvar = getenv ("HOME")) == 0) {
		return;
	}

	string path = string_compose("%1/.%2/rdf", envvar, domain);
	presets = scanner (path, rdf_filter, 0, true, true);

	if (presets) {
		for (x = presets->begin(); x != presets->end (); ++x) {
			string file = "file:" + **x;
			if (lrdf_read_file(file.c_str())) {
				warning << string_compose(_("Could not parse rdf file: %1"), *x) << endmsg;
			}
		}
	}

	vector_delete (presets);
}

void
PluginManager::add_lrdf_data (const string &path)
{
	PathScanner scanner;
	vector<string *>* rdf_files;
	vector<string *>::iterator x;
	string uri;

	rdf_files = scanner (path, rdf_filter, 0, true, true);

	if (rdf_files) {
		for (x = rdf_files->begin(); x != rdf_files->end (); ++x) {
			uri = "file://" + **x;

			if (lrdf_read_file(uri.c_str())) {
				warning << "Could not parse rdf file: " << uri << endmsg;
			}
		}
	}

	vector_delete (rdf_files);
}

int 
PluginManager::ladspa_discover (string path)
{
	void *module;
	const LADSPA_Descriptor *descriptor;
	LADSPA_Descriptor_Function dfunc;
	const char *errstr;

	if ((module = dlopen (path.c_str(), RTLD_NOW)) == 0) {
		error << string_compose(_("LADSPA: cannot load module \"%1\" (%2)"), path, dlerror()) << endmsg;
		return -1;
	}

	dfunc = (LADSPA_Descriptor_Function) dlsym (module, "ladspa_descriptor");

	if ((errstr = dlerror()) != 0) {
		error << string_compose(_("LADSPA: module \"%1\" has no descriptor function."), path) << endmsg;
		error << errstr << endmsg;
		dlclose (module);
		return -1;
	}

	for (uint32_t i = 0; ; ++i) {
		if ((descriptor = dfunc (i)) == 0) {
			break;
		}

		PluginInfoPtr info(new LadspaPluginInfo);
		info->name = descriptor->Name;
		info->category = get_ladspa_category(descriptor->UniqueID);
		info->path = path;
		info->index = i;
		info->n_inputs = 0;
		info->n_outputs = 0;
		info->type = PluginInfo::LADSPA;
		info->unique_id = descriptor->UniqueID;
		
		for (uint32_t n=0; n < descriptor->PortCount; ++n) {
			if ( LADSPA_IS_PORT_AUDIO (descriptor->PortDescriptors[n]) ) {
				if ( LADSPA_IS_PORT_INPUT (descriptor->PortDescriptors[n]) ) {
					info->n_inputs++;
				}
				else if ( LADSPA_IS_PORT_OUTPUT (descriptor->PortDescriptors[n]) ) {
					info->n_outputs++;
				}
			}
		}

		_ladspa_plugin_info.push_back (info);
	}

// GDB WILL NOT LIKE YOU IF YOU DO THIS
//	dlclose (module);

	return 0;
}

PluginPtr
ARDOUR::find_plugin(Session& session, string name, long unique_id, PluginInfo::Type type)
{
	PluginManager *mgr = PluginManager::the_manager();
	PluginInfoList plugs;

	switch (type) {
	case PluginInfo::LADSPA:
		plugs = mgr->ladspa_plugin_info();
		break;

#ifdef VST_SUPPORT
	case PluginInfo::VST:
		plugs = mgr->vst_plugin_info();
		unique_id = 0; // VST plugins don't have a unique id.
		break;
#endif

#ifdef HAVE_COREAUDIO
	case PluginInfo::AudioUnit:
		plugs = AUPluginInfo::discover ();
		unique_id = 0; // Neither do AU.
		break;
#endif

	default:
		return PluginPtr ((Plugin *) 0);
	}

	PluginInfoList::iterator i;
	for (i = plugs.begin(); i != plugs.end(); ++i) {
		if ((name == "" || (*i)->name == name) &&
			(unique_id == 0 || (*i)->unique_id == unique_id)) {
				return (*i)->load (session);
		}
	}
	
	return PluginPtr ((Plugin*) 0);
}

string
PluginManager::get_ladspa_category (uint32_t plugin_id)
{
	char buf[256];
	lrdf_statement pattern;

	snprintf(buf, sizeof(buf), "%s%" PRIu32, LADSPA_BASE, plugin_id);
	pattern.subject = buf;
	pattern.predicate = RDF_TYPE;
	pattern.object = 0;
	pattern.object_type = lrdf_uri;

	lrdf_statement* matches1 = lrdf_matches (&pattern);

	if (!matches1) {
		return _("Unknown");
	}

	pattern.subject = matches1->object;
	pattern.predicate = LADSPA_BASE "hasLabel";
	pattern.object = 0;
	pattern.object_type = lrdf_literal;

	lrdf_statement* matches2 = lrdf_matches (&pattern);
	lrdf_free_statements(matches1);

	if (!matches2) {
		return _("Unknown");
	}

	string label = matches2->object;
	lrdf_free_statements(matches2);

	return label;
}

#ifdef VST_SUPPORT

void
PluginManager::vst_refresh ()
{
	_vst_plugin_info.clear ();

	if (vst_path.length() == 0) {
		vst_path = "/usr/local/lib/vst:/usr/lib/vst";
	}

	vst_discover_from_path (vst_path);
}

int
PluginManager::add_vst_directory (string path)
{
	if (vst_discover_from_path (path) == 0) {
		vst_path += ':';
		vst_path += path;
		return 0;
	} 
	return -1;
}

static bool vst_filter (const string& str, void *arg)
{
	/* Not a dotfile, has a prefix before a period, suffix is "dll" */
	
	return str[0] != '.' && (str.length() > 4 && str.find (".dll") == (str.length() - 4));
}

int
PluginManager::vst_discover_from_path (string path)
{
	PathScanner scanner;
	vector<string *> *plugin_objects;
	vector<string *>::iterator x;
	int ret = 0;

	info << "detecting VST plugins along " << path << endmsg;

	plugin_objects = scanner (vst_path, vst_filter, 0, true, true);

	if (plugin_objects) {
		for (x = plugin_objects->begin(); x != plugin_objects->end (); ++x) {
			vst_discover (**x);
		}
	}

	vector_delete (plugin_objects);
	return ret;
}

int
PluginManager::vst_discover (string path)
{
	FSTInfo* finfo;

	if ((finfo = fst_get_info (const_cast<char *> (path.c_str()))) == 0) {
		return -1;
	}

	if (!finfo->canProcessReplacing) {
		warning << string_compose (_("VST plugin %1 does not support processReplacing, and so cannot be used in ardour at this time"),
				    finfo->name)
			<< endl;
	}
	
	PluginInfoPtr info(new PluginInfo);

	/* what a goddam joke freeware VST is */

	if (!strcasecmp ("The Unnamed plugin", finfo->name)) {
		info->name = PBD::basename_nosuffix (path);
	} else {
		info->name = finfo->name;
	}

	info->category = "VST";
	info->path = path;
	info->index = 0;
	info->n_inputs = finfo->numInputs;
	info->n_outputs = finfo->numOutputs;
	info->type = PluginInfo::VST;
	
	_vst_plugin_info.push_back (info);
	fst_free_info (finfo);

	return 0;
}

#endif // VST_SUPPORT
