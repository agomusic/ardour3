/*
    Copyright (C) 2001-2007 Paul Davis
    
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

#include <cstdlib>
#include <signal.h>

#include <sigc++/bind.h>
#include <gtkmm/settings.h>

#include <pbd/error.h>
#include <pbd/file_utils.h>
#include <pbd/textreceiver.h>
#include <pbd/failed_constructor.h>
#include <pbd/pthread_utils.h>

#include <jack/jack.h>

#include <ardour/svn_revision.h>
#include <ardour/version.h>
#include <ardour/ardour.h>
#include <ardour/audioengine.h>
#include <ardour/session_utils.h>
#include <ardour/filesystem_paths.h>

#include <gtkmm/main.h>
#include <gtkmm2ext/popup.h>
#include <gtkmm2ext/utils.h>

#include "version.h"
#include "utils.h"
#include "ardour_ui.h"
#include "opts.h"
#include "enums.h"

#include "i18n.h"

using namespace Gtk;
using namespace ARDOUR_COMMAND_LINE;
using namespace ARDOUR;
using namespace PBD;
using namespace sigc;

TextReceiver text_receiver ("ardour");

extern int curvetest (string);

static ARDOUR_UI  *ui = 0;
static const char* localedir = LOCALEDIR;

void
gui_jack_error ()
{
	MessageDialog win (_("Ardour could not connect to JACK."),
		     false,
		     Gtk::MESSAGE_INFO,
		     (Gtk::ButtonsType)(Gtk::BUTTONS_NONE));
win.set_secondary_text(_("There are several possible reasons:\n\
\n\
1) JACK is not running.\n\
2) JACK is running as another user, perhaps root.\n\
3) There is already another client called \"ardour\".\n\
\n\
Please consider the possibilities, and perhaps (re)start JACK."));

	win.add_button (Stock::QUIT, RESPONSE_CLOSE);
	win.set_default_response (RESPONSE_CLOSE);
	
	win.show_all ();
	win.set_position (Gtk::WIN_POS_CENTER);

	if (!no_splash) {
		ui->hide_splash ();
	}

	/* we just don't care about the result, but we want to block */

	win.run ();
}


#ifdef __APPLE__

#include <mach-o/dyld.h>
#include <sys/param.h>
#include <fstream>

void
fixup_bundle_environment ()
{
	if (!getenv ("ARDOUR_BUNDLED")) {
		return;
	}

	char execpath[MAXPATHLEN+1];
	uint32_t pathsz = sizeof (execpath);

	_NSGetExecutablePath (execpath, &pathsz);

	Glib::ustring exec_path (execpath);
	Glib::ustring dir_path = Glib::path_get_dirname (exec_path);
	Glib::ustring path;
	const char *cstr = getenv ("PATH");

	/* ensure that we find any bundled executables (e.g. JACK),
	   and find them before any instances of the same name
	   elsewhere in PATH
	*/

	path = dir_path;
	if (cstr) {
		path += ':';
		path += cstr;
	}
	setenv ("PATH", path.c_str(), 1);

	path = dir_path;
	path += "/../Resources";
	path += dir_path;
	path += "/../Resources/Surfaces";
	path += dir_path;
	path += "/../Resources/Panners";

	setenv ("ARDOUR_MODULE_PATH", path.c_str(), 1);

	path = dir_path;
	path += "/../Resources/icons:";
	path += dir_path;
	path += "/../Resources/pixmaps:";
	path += dir_path;
	path += "/../Resources/share:";
	path += dir_path;
	path += "/../Resources";

	setenv ("ARDOUR_PATH", path.c_str(), 1);
	setenv ("ARDOUR_CONFIG_PATH", path.c_str(), 1);
	setenv ("ARDOUR_DATA_PATH", path.c_str(), 1);

	path = dir_path;
	path += "/../Resources";
	setenv ("ARDOUR_INSTANT_XML_PATH", path.c_str(), 1);

	cstr = getenv ("LADSPA_PATH");
	if (cstr) {
		path = cstr;
		path += ':';
	} else {
		path = "";
	}
	path += dir_path;
	path += "/../Plugins";
	
	setenv ("LADSPA_PATH", path.c_str(), 1);

	cstr = getenv ("VAMP_PATH");
	if (cstr) {
		path = cstr;
		path += ':';
	} else {
		path = "";
	}
	path += dir_path;
	path += "/../Frameworks";
	
	setenv ("VAMP_PATH", path.c_str(), 1);

	cstr = getenv ("ARDOUR_CONTROL_SURFACE_PATH");
	if (cstr) {
		path = cstr;
		path += ':';
	} else {
		path = "";
	}
	path += dir_path;
	path += "/../Surfaces";
	
	setenv ("ARDOUR_CONTROL_SURFACE_PATH", path.c_str(), 1);

	cstr = getenv ("LV2_PATH");
	if (cstr) {
		path = cstr;
		path += ':';
	} else {
		path = "";
	}
	path += dir_path;
	path += "/../Plugins";
	
	setenv ("LV2_PATH", path.c_str(), 1);

	path = dir_path;
	path += "/../Frameworks/clearlooks";

	setenv ("GTK_PATH", path.c_str(), 1);

	path = dir_path;
	path += "/../Resources/locale";
	
	localedir = strdup (path.c_str());

	/* write a pango.rc file and tell pango to use it. we'd love
	   to put this into the Ardour.app bundle and leave it there,
	   but the user may not have write permission. so ... 

	   we also have to make sure that the user ardour directory
	   actually exists ...
	*/

	try {
		sys::create_directories (user_config_directory ());
	}
	catch (const sys::filesystem_error& ex) {
		error << _("Could not create user configuration directory") << endmsg;
	}
	
	sys::path pangopath = user_config_directory();
	pangopath /= "pango.rc";
	path = pangopath.to_string();

	std::ofstream pangorc (path.c_str());
	if (!pangorc) {
		error << string_compose (_("cannot open pango.rc file %1") , path) << endmsg;
		return;
	} else {
		pangorc << "[Pango]\nModuleFiles=";

		pangopath = dir_path;
		pangopath /= "..";
		pangopath /= "Resources";
		pangopath /= "pango.modules";
			
		pangorc << pangopath.to_string() << endl;

		pangorc.close ();

		setenv ("PANGO_RC_FILE", path.c_str(), 1);
	}

	// gettext charset aliases

	setenv ("CHARSETALIASDIR", path.c_str(), 1);

	// font config
	
	path = dir_path;
	path += "/../Resources/fonts.conf";

	setenv ("FONTCONFIG_FILE", path.c_str(), 1);

	// GDK Pixbuf loader module file

	path = dir_path;
	path += "/../Resources/gdk-pixbuf.loaders";

	setenv ("GDK_PIXBUF_MODULE_FILE", path.c_str(), 1);

	if (getenv ("ARDOUR_WITH_JACK")) {
		// JACK driver dir
		
		path = dir_path;
		path += "/../Frameworks";
		
		setenv ("JACK_DRIVER_DIR", path.c_str(), 1);
	}
}

#endif

static void
sigpipe_handler (int sig)
{
	cerr << _("SIGPIPE received - JACK has probably died") << endl;
}

#ifdef VST_SUPPORT
/* this is called from the entry point of a wine-compiled
   executable that is linked against gtk2_ardour built
   as a shared library.
*/
extern "C" {
int ardour_main (int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{
	vector<Glib::ustring> null_file_list;
	
#ifdef __APPLE__
	fixup_bundle_environment ();
#endif

        Glib::thread_init();
	gtk_set_locale ();

	(void) bindtextdomain (PACKAGE, localedir);
	/* our i18n translations are all in UTF-8, so make sure
	   that even if the user locale doesn't specify UTF-8,
	   we use that when handling them.
	*/
	(void) bind_textdomain_codeset (PACKAGE,"UTF-8");
	(void) textdomain (PACKAGE);

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	// catch error message system signals ();

	text_receiver.listen_to (error);
	text_receiver.listen_to (info);
	text_receiver.listen_to (fatal);
	text_receiver.listen_to (warning);

	if (parse_opts (argc, argv)) {
		exit (1);
	}

	if (curvetest_file) {
		return curvetest (curvetest_file);
	}
	
	cout << _("Ardour/GTK ") 
	     << VERSIONSTRING
	     << _("\n   (built using ")
	     << svn_revision
#ifdef __GNUC__
	     << _(" and GCC version ") << __VERSION__ 
#endif
	     << ')'
	     << endl;
	
	if (just_version) {
		exit (0);
	}

	if (no_splash) {
		cerr << _("Copyright (C) 1999-2008 Paul Davis") << endl
		     << _("Some portions Copyright (C) Steve Harris, Ari Johnson, Brett Viren, Joel Baker") << endl
		     << endl
		     << _("Ardour comes with ABSOLUTELY NO WARRANTY") << endl
		     <<	_("not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.") << endl
		     << _("This is free software, and you are welcome to redistribute it ") << endl
		     << _("under certain conditions; see the source for copying conditions.")
		     << endl;
	}

	/* some GUI objects need this */

	PBD::ID::init ();

	if (::signal (SIGPIPE, sigpipe_handler)) {
		cerr << _("Cannot install SIGPIPE error handler") << endl;
	}

        try { 
		ui = new ARDOUR_UI (&argc, &argv);
	} catch (failed_constructor& err) {
		error << _("could not create ARDOUR GUI") << endmsg;
		exit (1);
	}

	ui->run (text_receiver);
	ui = 0;

	ARDOUR::cleanup ();
	pthread_cancel_all ();
	return 0;
}
#ifdef VST_SUPPORT
} // end of extern C block
#endif

