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

*/

#define __STDC_FORMAT_MACROS 1
#include <stdint.h>

#include <sys/types.h>
#include <cstdio>
#include <lrdf.h>
#include <dlfcn.h>
#include <cstdlib>
#include <fstream>

#ifdef VST_SUPPORT
#include <fst.h>
#include <pbd/basename.h>
#include <cstring>
#endif // VST_SUPPORT

#include <glibmm/miscutils.h>

#include <pbd/pathscanner.h>

#include <ardour/ladspa.h>
#include <ardour/session.h>
#include <ardour/plugin_manager.h>
#include <ardour/plugin.h>
#include <ardour/ladspa_plugin.h>
#include <ardour/filesystem_paths.h>

#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#include <ardour/lv2_plugin.h>
#endif

#ifdef VST_SUPPORT
#include <ardour/vst_plugin.h>
#endif

#ifdef HAVE_AUDIOUNITS
#include <ardour/audio_unit.h>
#include <Carbon/Carbon.h>
#endif

#include <pbd/error.h>
#include <pbd/stl_delete.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace std;

PluginManager* PluginManager::_manager = 0;

PluginManager::PluginManager ()
{
	char* s;
	string lrdf_path;

	load_favorites ();

#ifdef GTKOSX
	ProcessSerialNumber psn = { 0, kCurrentProcess }; 
	OSStatus returnCode = TransformProcessType(& psn, kProcessTransformToForegroundApplication);
	if( returnCode != 0) {
		error << _("Cannot become GUI app") << endmsg;
	}
#endif

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

	if (_manager == 0) {
		_manager = this;
	}

	/* the plugin manager is constructed too early to use Profile */

	if (getenv ("ARDOUR_SAE")) {
		ladspa_plugin_whitelist.push_back (1203); // single band parametric
		ladspa_plugin_whitelist.push_back (1772); // caps compressor
		ladspa_plugin_whitelist.push_back (1913); // fast lookahead limiter
		ladspa_plugin_whitelist.push_back (1075); // simple RMS expander
		ladspa_plugin_whitelist.push_back (1061); // feedback delay line (max 5s)
		ladspa_plugin_whitelist.push_back (1216); // gverb
		ladspa_plugin_whitelist.push_back (2150); // tap pitch shifter
	} 

#ifdef HAVE_SLV2
	_lv2_world = new LV2World();
#endif

	BootMessage (_("Discovering Plugins"));

	refresh ();
}

void
PluginManager::refresh ()
{
	ladspa_refresh ();
#ifdef HAVE_SLV2
	lv2_refresh ();
#endif
#ifdef VST_SUPPORT
	if (Config->get_use_vst()) {
		vst_refresh ();
	}
#endif // VST_SUPPORT
#ifdef HAVE_AUDIOUNITS
	au_refresh ();
#endif
}

void
PluginManager::ladspa_refresh ()
{
  	_ladspa_plugin_info.clear ();
	
 	static const char *standard_paths[] = {
 		"/usr/local/lib64/ladspa",
 		"/usr/local/lib/ladspa",
 		"/usr/lib64/ladspa",
 		"/usr/lib/ladspa",
 		"/Library/Audio/Plug-Ins/LADSPA",
 		""
 	};
 	
 	/* allow LADSPA_PATH to augment, not override standard locations */
 
 	/* Only add standard locations to ladspa_path if it doesn't
 	 * already contain them. Check for trailing '/'s too.
 	 */
 	 
 	int i;
 	for (i = 0; standard_paths[i][0]; i++) {
 		size_t found = ladspa_path.find(standard_paths[i]);
 		if (found != ladspa_path.npos) {
 			switch (ladspa_path[found + strlen(standard_paths[i])]) {
 				case ':' :
 				case '\0':
 					continue;
 				case '/' :
 					if (ladspa_path[found + strlen(standard_paths[i]) + 1] == ':' ||
 					    ladspa_path[found + strlen(standard_paths[i]) + 1] == '\0') {
 						continue;
 					}
 			}
 		}
 		if (!ladspa_path.empty())
 			ladspa_path += ":";
 
 		ladspa_path += standard_paths[i]; 
 		
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

		if (!ladspa_plugin_whitelist.empty()) {
			if (find (ladspa_plugin_whitelist.begin(), ladspa_plugin_whitelist.end(), descriptor->UniqueID) == ladspa_plugin_whitelist.end()) {
				continue;
			}
		} 

		PluginInfoPtr info(new LadspaPluginInfo);
		info->name = descriptor->Name;
		info->category = get_ladspa_category(descriptor->UniqueID);
		info->creator = descriptor->Maker;
		info->path = path;
		info->index = i;
		info->n_inputs = ChanCount();
		info->n_outputs = ChanCount();
		info->type = ARDOUR::LADSPA;
		
		char buf[32];
		snprintf (buf, sizeof (buf), "%lu", descriptor->UniqueID);
		info->unique_id = buf;
		
		for (uint32_t n=0; n < descriptor->PortCount; ++n) {
			if ( LADSPA_IS_PORT_AUDIO (descriptor->PortDescriptors[n]) ) {
				if ( LADSPA_IS_PORT_INPUT (descriptor->PortDescriptors[n]) ) {
					info->n_inputs.set_audio(info->n_inputs.n_audio() + 1);
				}
				else if ( LADSPA_IS_PORT_OUTPUT (descriptor->PortDescriptors[n]) ) {
					info->n_outputs.set_audio(info->n_outputs.n_audio() + 1);
				}
			}
		}

		_ladspa_plugin_info.push_back (info);
	}

// GDB WILL NOT LIKE YOU IF YOU DO THIS
//	dlclose (module);

	return 0;
}

string
PluginManager::get_ladspa_category (uint32_t plugin_id)
{
	char buf[256];
	lrdf_statement pattern;

	snprintf(buf, sizeof(buf), "%s%" PRIu32, LADSPA_BASE, plugin_id);
	pattern.subject = buf;
	pattern.predicate = (char*)RDF_TYPE;
	pattern.object = 0;
	pattern.object_type = lrdf_uri;

	lrdf_statement* matches1 = lrdf_matches (&pattern);

	if (!matches1) {
		return "";
	}

	pattern.subject = matches1->object;
	pattern.predicate = (char*)(LADSPA_BASE "hasLabel");
	pattern.object = 0;
	pattern.object_type = lrdf_literal;

	lrdf_statement* matches2 = lrdf_matches (&pattern);
	lrdf_free_statements(matches1);

	if (!matches2) {
		return ("");
	}

	string label = matches2->object;
	lrdf_free_statements(matches2);

	return label;
}

#ifdef HAVE_SLV2
void
PluginManager::lv2_refresh ()
{
	lv2_discover();
}

int
PluginManager::lv2_discover ()
{
	_lv2_plugin_info = LV2PluginInfo::discover(_lv2_world);
	return 0;
}
#endif

#ifdef HAVE_AUDIOUNITS
void
PluginManager::au_refresh ()
{
	au_discover();
}

int
PluginManager::au_discover ()
{
	_au_plugin_info = AUPluginInfo::discover();
	return 0;
}

#endif

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
	char buf[32];

	if ((finfo = fst_get_info (const_cast<char *> (path.c_str()))) == 0) {
		warning << "Cannot get VST information from " << path << endmsg;
		return -1;
	}

	if (!finfo->canProcessReplacing) {
		warning << string_compose (_("VST plugin %1 does not support processReplacing, and so cannot be used in ardour at this time"),
				    finfo->name)
			<< endl;
	}
	
	PluginInfoPtr info(new VSTPluginInfo);

	/* what a joke freeware VST is */

	if (!strcasecmp ("The Unnamed plugin", finfo->name)) {
		info->name = PBD::basename_nosuffix (path);
	} else {
		info->name = finfo->name;
	}

	
	snprintf (buf, sizeof (buf), "%d", finfo->UniqueID);
	info->unique_id = buf;
	info->category = "VST";
	info->path = path;
	// need to set info->creator but FST doesn't provide it
	info->index = 0;
	info->n_inputs = finfo->numInputs;
	info->n_outputs = finfo->numOutputs;
	info->type = ARDOUR::VST;
	
	_vst_plugin_info.push_back (info);
	fst_free_info (finfo);

	return 0;
}

#endif // VST_SUPPORT

bool
PluginManager::is_a_favorite_plugin (const PluginInfoPtr& pi)
{
	FavoritePlugin fp (pi->type, pi->unique_id);
	return find (favorites.begin(), favorites.end(), fp) !=  favorites.end();
}

void
PluginManager::save_favorites ()
{
	ofstream ofs;
	sys::path path = user_config_directory();
	path /= "favorite_plugins";

	ofs.open (path.to_string().c_str(), ios_base::openmode (ios::out|ios::trunc));

	if (!ofs) {
		return;
	}

	for (FavoritePluginList::iterator i = favorites.begin(); i != favorites.end(); ++i) {
		switch ((*i).type) {
		case LADSPA:
			ofs << "LADSPA";
			break;
		case AudioUnit:
			ofs << "AudioUnit";
			break;
		case LV2:
			ofs << "LV2";
			break;
		case VST:
			ofs << "VST";
			break;
		}
		
		ofs << ' ' << (*i).unique_id << endl;
	}

	ofs.close ();
}

void
PluginManager::load_favorites ()
{
	sys::path path = user_config_directory();
	path /= "favorite_plugins";
	ifstream ifs (path.to_string().c_str());

	if (!ifs) {
		return;
	}
	
	std::string stype;
	std::string id;
	PluginType type;

	while (ifs) {

		ifs >> stype;
		if (!ifs) {
			break;

		}
		ifs >> id;
		if (!ifs) {
			break;
		}

		if (stype == "LADSPA") {
			type = LADSPA;
		} else if (stype == "AudioUnit") {
			type = AudioUnit;
		} else if (stype == "LV2") {
			type = LV2;
		} else if (stype == "VST") {
			type = VST;
		} else {
			error << string_compose (_("unknown favorite plugin type \"%1\" - ignored"), stype)
			      << endmsg;
			continue;
		}
		
		add_favorite (type, id);
	}
	
	ifs.close ();
}

void
PluginManager::add_favorite (PluginType t, string id)
{
	FavoritePlugin fp (t, id);
	pair<FavoritePluginList::iterator,bool> res = favorites.insert (fp);
	//cerr << "Added " << t << " " << id << " success ? " << res.second << endl;
}

void
PluginManager::remove_favorite (PluginType t, string id)
{
	FavoritePlugin fp (t, id);
	favorites.erase (fp);
}
