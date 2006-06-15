/*
    Copyright (C) 2001-2006 Paul Davis
    
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
#include <sys/mman.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include <sigc++/bind.h>
#include <gtkmm/settings.h>

#include <pbd/error.h>
#include <pbd/textreceiver.h>
#include <pbd/failed_constructor.h>
#include <pbd/pthread_utils.h>

#include <jack/jack.h>

#include <ardour/version.h>
#include <ardour/ardour.h>
#include <ardour/audioengine.h>

#include <gtkmm/main.h>
#include <gtkmm2ext/popup.h>
#include <gtkmm2ext/utils.h>

#include "version.h"
#include "ardour_ui.h"
#include "opts.h"

#include "i18n.h"

using namespace Gtk;
using namespace GTK_ARDOUR;
using namespace ARDOUR;
using namespace sigc;

TextReceiver text_receiver ("ardour");

extern int curvetest (string);

static ARDOUR_UI  *ui = 0;

static void
shutdown (int status)
{
	char* msg;

	if (status) {

		msg = _("ardour is killing itself for a clean exit\n");
		write (1, msg, strlen (msg));
		/* drastic, but perhaps necessary */
		kill (-getpgrp(), SIGKILL);	
		/*NOTREACHED*/

	} else {

		if (ui) {
			msg = _("stopping user interface\n");
			write (1, msg, strlen (msg));
			ui->kill();
		}
		
		pthread_cancel_all ();
	}

	exit (status);
}


static void 
handler (int sig)
{
	char buf[64];
	int n;

	/* XXX its doubtful that snprintf() is async-safe */
	n = snprintf (buf, sizeof(buf), _("%d(%d): received signal %d\n"), getpid(), (int) pthread_self(), sig);
	write (1, buf, n);

	shutdown (1);
}

static void 
handler2 (int sig, siginfo_t* ctxt, void* ignored)
{
	handler (sig);
}	

static void *
signal_thread (void *arg)
{
	int sig;
	sigset_t blocked;

	PBD::ThreadCreated (pthread_self(), X_("Signal"));

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	
	/* find out what's blocked right now */

	//sigprocmask (SIG_SETMASK, 0, &blocked);
	if (pthread_sigmask (SIG_SETMASK, 0, &blocked)) {
		cerr << "getting blocked signals failed\n";
	}
	
	/* wait for any of the currently blocked signals.
	   
	   According to the man page found in linux 2.6 and 2.4, sigwait() 
	   never returns an error. This is incorrect. Checking the man
	   pages for some other *nix systems makes it clear that
	   sigwait() can return several error codes, one of which 
	   is EINTR. This happens if the thread receives a signal
	   which is not in the blocked set. 

	   We do not expect that to happen, and if it did we should generally
	   exit as planned. However, under 2.6, the ptrace facility used 
	   by gdb seems to also cause sigwait() to return with EINTR
	   but with a signal that sigwait cannot understand. As a result, 
	   "sig" is set to zero, an impossible signal number.

	   Handling the EINTR code makes it possible to debug 
	   ardour on a 2.6 kernel.

	*/

	int swerr;

  again:
	if ((swerr = sigwait (&blocked, &sig))) {
		if (swerr == EINTR) {
			goto again;
		} else {
			cerr << "sigwait failed with " << swerr << endl;
		}
	}

	cerr << "Signal " << sig << " received\n";

	if (sig != SIGSEGV) {

		/* unblock signals so we can see them during shutdown.
		   this will help prod developers not to lose sight
		   of bugs that cause segfaults etc. during shutdown.
		*/

		sigprocmask (SIG_UNBLOCK, &blocked, 0);
	}

	shutdown (1);
	/*NOTREACHED*/
	return 0;
}

int
catch_signals (void)
{
	struct sigaction action;
	pthread_t signal_thread_id;
	sigset_t signals;

//	if (setpgid (0,0)) {
	if (setsid ()) {
		warning << string_compose (_("cannot become new process group leader (%1)"), 
				    strerror (errno))
			<< endmsg;
	}

	sigemptyset (&signals);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGQUIT);
	sigaddset(&signals, SIGPIPE);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);


	/* install a handler because otherwise
	   pthreads behaviour is undefined when we enter
	   sigwait.
	*/
	
	action.sa_handler = handler;
	action.sa_mask = signals;
	action.sa_flags = SA_RESTART|SA_RESETHAND;

	for (int i = 1; i < 32; i++) {
		if (sigismember (&signals, i)) {
			if (sigaction (i, &action, 0)) {
				cerr << string_compose (_("cannot setup signal handling for %1"), i) << endl;
				return -1;
			}
		}
	} 

	/* this sets the signal mask for this and all 
	   subsequent threads that do not reset it.
	*/
	
	if (pthread_sigmask (SIG_SETMASK, &signals, 0)) {
		cerr << string_compose (_("cannot set default signal mask (%1)"), strerror (errno)) << endl;
		return -1;
	}

	/* start a thread to wait for signals */

	if (pthread_create_and_store ("signal", &signal_thread_id, 0, signal_thread, 0)) {
		cerr << "cannot create signal catching thread" << endl;
		return -1;
	}

	pthread_detach (signal_thread_id);
	return 0;
}

string
which_ui_rcfile ()
{
	string rcfile;
	char* env;

	if ((env = getenv ("ARDOUR2_UI_RC")) != 0 && strlen (env)) {
		rcfile = env;
	} else {
		rcfile = "ardour2_ui.rc";
	}

	rcfile = find_config_file (rcfile);
	
	if (rcfile.empty()) {
		warning << _("Without a UI style file, ardour will look strange.\n Please set ARDOUR2_UI_RC to point to a valid UI style file") << endmsg;
	} else {
		cerr << "Loading ui configuration file " << rcfile << endl;
	}
	
	return rcfile;
}

gint
show_ui_callback (void *arg)
{
 	ARDOUR_UI * ui = (ARDOUR_UI *) arg;

	ui->hide_splash();
	ui->show ();
	
	return FALSE;
}

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

	/* we just don't care about the result */

	win.run ();
}

static bool
maybe_load_session ()
{

	/* load session, if given */
	string name, path;

	if (session_name.length()){
		bool isnew;

		if (Session::find_session (session_name, path, name, isnew)) {
			error << string_compose(_("could not load command line session \"%1\""), session_name) << endmsg;
		} else {

			if (new_session) {

				/* command line required that the session be new */

				if (isnew) {
					
					/* popup the new session dialog
					   once everything else is OK.
					*/

					Glib::signal_idle().connect (bind (mem_fun (*ui, &ARDOUR_UI::cmdline_new_session), path));
					ui->set_will_create_new_session_automatically (true);

				} else {

					/* it wasn't new, but we require a new session */

					error << string_compose (_("\n\nA session named \"%1\" already exists.\n\
To avoid this message, start ardour as \"ardour %1"), path)
					      << endmsg;
					return false;
				}

			} else {

				/* command line didn't require a new session */
				
				if (isnew) {
					error << string_compose (_("\n\nNo session named \"%1\" exists.\n\
To create it from the command line, start ardour as \"ardour --new %1"), path) 
					      << endmsg;
					return false;
				}

				ui->load_session (path, name);
			}
		}

		if (no_splash) {
			ui->show();
		}

	} else {
		ui->hide_splash ();
		if (!Config->get_no_new_session_dialog()) {
		       ui->new_session (true);
		}
	}

	return true;
}

int
main (int argc, char *argv[])
{
	ARDOUR::AudioEngine *engine;
	vector<Glib::ustring> null_file_list;

        // needs a better home.
        Glib::thread_init();
        
	gtk_set_locale ();

	(void)   bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	// catch_signals ();

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
	     << gtk_ardour_major_version << '.'
	     << gtk_ardour_minor_version << '.'
	     << gtk_ardour_micro_version
	     << _(" with libardour ")
	     << libardour_major_version << '.'
	     << libardour_minor_version << '.' 
	     << libardour_micro_version 
#ifdef __GNUC__
	     << _(" and GCC version ") << __VERSION__ 
#endif
	     << ')'
	     << endl;
	
	if (just_version) {
		exit (0);
	}

	if (no_splash) {
		cerr << _("Copyright (C) 1999-2006 Paul Davis") << endl
		     << _("Some portions Copyright (C) Steve Harris, Ari Johnson, Brett Viren, Joel Baker") << endl
		     << endl
		     << _("Ardour comes with ABSOLUTELY NO WARRANTY") << endl
		     <<	_("not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.") << endl
		     << _("This is free software, and you are welcome to redistribute it ") << endl
		     << _("under certain conditions; see the source for copying conditions.")
		     << endl;
	}

	try { 
		ui = new ARDOUR_UI (&argc, &argv, which_ui_rcfile());
	} 

	catch (failed_constructor& err) {
		error << _("could not create ARDOUR GUI") << endmsg;
		exit (1);
	}


	if (!no_splash) {
		ui->show_splash ();
		if (session_name.length()) {  
			gtk_timeout_add (4000, show_ui_callback, ui);
		}
	}
	
	try { 
		engine = new ARDOUR::AudioEngine (jack_client_name);
		ARDOUR::init (*engine, use_vst, try_hw_optimization, handler2);
		ui->set_engine (*engine);
	} catch (AudioEngine::NoBackendAvailable& err) {
		gui_jack_error ();
		error << string_compose (_("Could not connect to JACK server as  \"%1\""), jack_client_name) <<  endmsg;
		return -1;
	} catch (failed_constructor& err) {
		error << _("could not initialize Ardour.") << endmsg;
		return -1;
	} 

	if (maybe_load_session ()) {
		ui->run (text_receiver);
		ui = 0;
	}

	delete engine;
	ARDOUR::cleanup ();
	shutdown (0);

	/* just another commit forcing change */
}

