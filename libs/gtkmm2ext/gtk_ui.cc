/*
    Copyright (C) 1999-2005 Paul Barton-Davis 

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

#include <cmath>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cctype>

#include <gtkmm.h>
#include <pbd/error.h>
#include <pbd/touchable.h>
#include <pbd/failed_constructor.h>
#include <pbd/pthread_utils.h>

#include <gtkmm2ext/gtk_ui.h>
#include <gtkmm2ext/textviewer.h>
#include <gtkmm2ext/popup.h>
#include <gtkmm2ext/utils.h>

#include "i18n.h"

using namespace Gtkmm2ext;
using std::map;

pthread_t UI::gui_thread;
UI       *UI::theGtkUI = 0;

UI::UI (string name, int *argc, char ***argv, string rcfile) 
	: _ui_name (name)
{
	theMain = new Gtk::Main (argc, argv);
	tips = new Gtk::Tooltips;

	if (pthread_key_create (&thread_request_buffer_key, 0)) {
		cerr << _("cannot create thread request buffer key") << endl;
		throw failed_constructor();
	}

	PBD::ThreadCreated.connect (mem_fun (*this, &UI::register_thread));

	_ok = false;
	_active = false;

	if (!theGtkUI) {
		theGtkUI = this;
		gui_thread = pthread_self ();
	} else {
		fatal << "duplicate UI requested" << endmsg;
		/* NOTREACHED */
	}

	if (setup_signal_pipe ()) {
		return;
	}

	load_rcfile (rcfile);

	errors = new TextViewer (850,100);
	errors->text().set_editable (false); 
	errors->text().set_name ("ErrorText");

	string title;
	title = _ui_name;
	title += ": Log";
	errors->set_title (title);

	errors->dismiss_button().set_name ("ErrorLogCloseButton");
//	errors->realize();

	Glib::RefPtr<Gdk::Window> win(errors->get_window());
	win->set_decorations (Gdk::WMDecoration (Gdk::DECOR_BORDER|Gdk::DECOR_RESIZEH));

	errors->signal_delete_event().connect (bind (ptr_fun (just_hide_it), (Gtk::Window *) errors));

	register_thread (pthread_self(), X_("GUI"));

	_ok = true;
}

UI::~UI ()
{
	close (signal_pipe[0]);
	close (signal_pipe[1]);
}

int
UI::load_rcfile (string path)
{
	if (path.length() == 0) {
		return -1;
	}

	if (access (path.c_str(), R_OK)) {
		error << "UI: couldn't find rc file \"" 
		      << path
		      << '"'
		      << endmsg;
		return -1;
	}
	
	gtk_rc_parse (path.c_str());

	Gtk::Label *a_widget1;
	Gtk::Label *a_widget2;
	Gtk::Label *a_widget3;
	Gtk::Label *a_widget4;

	a_widget1 = new Gtk::Label;
	a_widget2 = new Gtk::Label;
	a_widget3 = new Gtk::Label;
	a_widget4 = new Gtk::Label;

	a_widget1->set_name ("FatalMessage");
	a_widget1->ensure_style ();
	fatal_message_style = a_widget1->get_style();

	a_widget2->set_name ("ErrorMessage");
	a_widget2->ensure_style ();
	error_message_style = a_widget2->get_style();

	a_widget3->set_name ("WarningMessage");
	a_widget3->ensure_style ();
	warning_message_style = a_widget3->get_style();

	a_widget4->set_name ("InfoMessage");
	a_widget4->ensure_style ();
	info_message_style = a_widget4->get_style();

	delete a_widget1;
	delete a_widget2;
	delete a_widget3;
	delete a_widget4;

	return 0;
}

void
UI::run (Receiver &old_receiver)
{
	listen_to (error);
	listen_to (info);
	listen_to (warning);
	listen_to (fatal);

	old_receiver.hangup ();
	starting ();
	_active = true;	
	theMain->run ();
	_active = false;
	stopping ();
	hangup ();
	return;
}

bool
UI::running ()
{
	return _active;
}

void
UI::kill ()
{
	if (_active) {
		pthread_kill (gui_thread, SIGKILL);
	} 
}

void
UI::quit ()
{
	request (Quit);
}

void
UI::do_quit ()
{
	Gtk::Main::quit();
}

void
UI::touch_display (Touchable *display)
{
	Request *req = get_request (TouchDisplay);

	if (req == 0) {
		return;
	}

	req->display = display;

	send_request (req);
}	

void
UI::call_slot (sigc::slot<void> slot)
{
	Request *req = get_request (CallSlot);

	if (req == 0) {
		return;
	}

	req->slot = slot;

	send_request (req);
}	

void
UI::call_slot_locked (sigc::slot<void> slot)
{
	if (caller_is_gui_thread()) {
		call_slot (slot);
		return;
	}

	Request *req = get_request (CallSlotLocked);

	if (req == 0) {
		return;
	}

	req->slot = slot;

	pthread_mutex_init (&req->slot_lock, NULL);
	pthread_cond_init (&req->slot_cond, NULL);
	pthread_mutex_lock (&req->slot_lock);

	send_request (req);

	pthread_cond_wait (&req->slot_cond, &req->slot_lock);
	pthread_mutex_unlock (&req->slot_lock);

	delete req;
}	

void
UI::set_tip (Gtk::Widget *w, const gchar *tip, const gchar *hlp)
{
	Request *req = get_request (SetTip);

	if (req == 0) {
		return;
	}

	req->widget = w;
	req->msg = tip;
	req->msg2 = hlp;

	send_request (req);
}

void
UI::set_state (Gtk::Widget *w, Gtk::StateType state)
{
	Request *req = get_request (StateChange);
	
	if (req == 0) {
		return;
	}

	req->new_state = state;
	req->widget = w;

	send_request (req);
}

void
UI::idle_add (int (*func)(void *), void *arg)
{
	Request *req = get_request (AddIdle);

	if (req == 0) {
		return;
	}

	req->function = func;
	req->arg = arg;

	send_request (req);
}

void
UI::timeout_add (unsigned int timeout, int (*func)(void *), void *arg)
{
	Request *req = get_request (AddTimeout);

	if (req == 0) {
		return;
	}

	req->function = func;
	req->arg = arg;
	req->timeout = timeout;

	send_request (req);
}

/* END abstract_ui interfaces */

/* Handling requests */

void
UI::register_thread (pthread_t thread_id, string name)
{
	RingBufferNPT<Request>* b = new RingBufferNPT<Request> (128);

	{
		PBD::LockMonitor lm (request_buffer_map_lock, __LINE__, __FILE__);
		request_buffers[thread_id] = b;
	}

	pthread_setspecific (thread_request_buffer_key, b);
}

UI::Request::Request()
{

}

UI::Request*
UI::get_request (RequestType rt)
{
	RingBufferNPT<Request>* rbuf = static_cast<RingBufferNPT<Request>* >(pthread_getspecific (thread_request_buffer_key));

	if (rbuf == 0) {
		/* Cannot happen, but if it does we can't use the error reporting mechanism */
		cerr << _("programming error: ")
		     << string_compose (X_("no GUI request buffer found for thread %1"), pthread_self())
		     << endl;
		abort ();
	}
	
	RingBufferNPT<Request>::rw_vector vec;
	
	rbuf->get_write_vector (&vec);

	if (vec.len[0] == 0) {
		if (vec.len[1] == 0) {
			cerr << string_compose (X_("no space in GUI request buffer for thread %1"), pthread_self())
			     << endl;
			return 0;
		} else {
			vec.buf[1]->type = rt;
			return vec.buf[1];
		}
	} else {
		vec.buf[0]->type = rt;
		return vec.buf[0];
	}
}

int
UI::setup_signal_pipe ()
{
	/* setup the pipe that other threads send us notifications/requests
	   through.
	*/

	if (pipe (signal_pipe)) {
		error << "UI: cannot create error signal pipe ("
		      << strerror (errno) << ")" 
		      << endmsg;

		return -1;
	}

	if (fcntl (signal_pipe[0], F_SETFL, O_NONBLOCK)) {
		error << "UI: cannot set O_NONBLOCK on "
			 "signal read pipe ("
		      << strerror (errno) << ")"
		      << endmsg;
		return -1;
	}

	if (fcntl (signal_pipe[1], F_SETFL, O_NONBLOCK)) {
		error << "UI: cannot set O_NONBLOCK on "
			 "signal write pipe ("
		      << strerror (errno) 
		      << ")" 
		      << endmsg;
		return -1;
	}

	/* add the pipe to the select/poll loop that GDK does */

	gdk_input_add (signal_pipe[0],
		       GDK_INPUT_READ,
		       UI::signal_pipe_callback,
		       this);

	return 0;
}

void
UI::signal_pipe_callback (void *arg, int fd, GdkInputCondition cond)
{
	char buf[256];
	
	/* flush (nonblocking) pipe */
	
	while (read (fd, buf, 256) > 0);
	
	((UI *) arg)->handle_ui_requests ();
}

void
UI::handle_ui_requests ()
{
	RequestBufferMap::iterator i;

	request_buffer_map_lock.lock ();

	for (i = request_buffers.begin(); i != request_buffers.end(); ++i) {

		RingBufferNPT<Request>::rw_vector vec;

		while (true) {

			/* we must process requests 1 by 1 because
			   the request may run a recursive main
			   event loop that will itself call
			   handle_ui_requests. when we return
			   from the request handler, we cannot
			   expect that the state of queued requests
			   is even remotely consistent with
			   the condition before we called it.
			*/

			i->second->get_read_vector (&vec);

			if (vec.len[0] == 0) {
				break;
			} else {
				/* copy constructor does a deep
				   copy of the Request object,
				   unlike Ringbuffer::read()
				*/
				Request req (*vec.buf[0]);
				i->second->increment_read_ptr (1);
				request_buffer_map_lock.unlock ();
				do_request (&req);
				request_buffer_map_lock.lock ();
			} 
		}
	}

	request_buffer_map_lock.unlock ();
}

void
UI::do_request (Request* req)
{
	switch (req->type) {
	case ErrorMessage:
		process_error_message (req->chn, req->msg);
		free (const_cast<char*>(req->msg)); /* it was strdup'ed */
		break;
		
	case Quit:
		do_quit ();
		break;
		
	case CallSlot:
		req->slot ();
		break;

	case CallSlotLocked:
		pthread_mutex_lock (&req->slot_lock);
		req->slot ();
		pthread_cond_signal (&req->slot_cond);
		pthread_mutex_unlock (&req->slot_lock);
		break;
		
	case TouchDisplay:
		req->display->touch ();
		if (req->display->delete_after_touch()) {
			delete req->display;
		}
		break;
		
	case StateChange:
		req->widget->set_state (req->new_state);
		break;
		
	case SetTip:
		/* XXX need to figure out how this works */
		break;
		
	case AddIdle:
		gtk_idle_add (req->function, req->arg);
		break;
		
	case AddTimeout:
		gtk_timeout_add (req->timeout, req->function, req->arg);
		break;
		
	default:
		error << "UI: unknown request type "
		      << (int) req->type
		      << endmsg;
	}	       
}

void
UI::send_request (Request *req)
{
	if (instance() == 0) {
		return; /* XXX is this the right thing to do ? */
	}
	
	if (caller_is_gui_thread()) {
		// cerr << "GUI thread sent request " << req << " type = " << req->type << endl;
		do_request (req);
	} else {	
		const char c = 0;
		RingBufferNPT<Request*>* rbuf = static_cast<RingBufferNPT<Request*> *> (pthread_getspecific (thread_request_buffer_key));

		if (rbuf == 0) {
			/* can't use the error system to report this, because this
			   thread isn't registered!
			*/
			cerr << _("programming error: ")
			     << string_compose (X_("UI::send_request() called from %1, but no request buffer exists for that thread"),
					 pthread_self())
			     << endl;
			abort ();
		}
		
		// cerr << "thread " << pthread_self() << " sent request " << req << " type = " << req->type << endl;
		rbuf->increment_write_ptr (1);
		write (signal_pipe[1], &c, 1);
	}
}

void
UI::request (RequestType rt)
{
	Request *req = get_request (rt);

	if (req == 0) {
		return;
	}

	send_request (req);
}

/*======================================================================
  Error Display
  ======================================================================*/

void
UI::receive (Transmitter::Channel chn, const char *str)
{
	if (caller_is_gui_thread()) {
		process_error_message (chn, str);
	} else {
		Request* req = get_request (ErrorMessage);

		if (req == 0) {
			return;
		}

		req->chn = chn;
		req->msg = strdup (str);

		send_request (req);
	}
}

#define OLD_STYLE_ERRORS 1

void
UI::process_error_message (Transmitter::Channel chn, const char *str)
{
	Glib::RefPtr<Gtk::Style> style;
	char *prefix;
	size_t prefix_len;
	bool fatal_received = false;
#ifndef OLD_STYLE_ERRORS
	PopUp* popup = new PopUp (Gtk::WIN_POS_CENTER, 0, true);
#endif

	switch (chn) {
	case Transmitter::Fatal:
		prefix = "[FATAL]: ";
		style = fatal_message_style;
		prefix_len = 9;
		fatal_received = true;
		break;
	case Transmitter::Error:
#if OLD_STYLE_ERRORS
		prefix = "[ERROR]: ";
		style = error_message_style;
		prefix_len = 9;
#else
		popup->set_name ("ErrorMessage");
		popup->set_text (str);
		popup->touch ();
		return;
#endif
		break;
	case Transmitter::Info:
#if OLD_STYLE_ERRORS	
		prefix = "[INFO]: ";
		style = info_message_style;
		prefix_len = 8;
#else
		popup->set_name ("InfoMessage");
		popup->set_text (str);
		popup->touch ();
		return;
#endif

		break;
	case Transmitter::Warning:
#if OLD_STYLE_ERRORS
		prefix = "[WARNING]: ";
		style = warning_message_style;
		prefix_len = 11;
#else
		popup->set_name ("WarningMessage");
		popup->set_text (str);
		popup->touch ();
		return;
#endif
		break;
	default:
		/* no choice but to use text/console output here */
		cerr << "programmer error in UI::check_error_messages (channel = " << chn << ")\n";
		::exit (1);
	}
	
	errors->text().get_buffer()->begin_user_action();

	if (fatal_received) {
		handle_fatal (str);
	} else {
		
		display_message (prefix, prefix_len, style, str);
		
		if (!errors->is_visible()) {
			toggle_errors();
		}
	}

	errors->text().get_buffer()->end_user_action();
}

void
UI::toggle_errors ()
{
	if (!errors->is_visible()) {
		errors->set_position (Gtk::WIN_POS_MOUSE);
		errors->show ();
	} else {
		errors->hide ();
	}
}

void
UI::display_message (const char *prefix, gint prefix_len, Glib::RefPtr<Gtk::Style> style, const char *msg)
{
	Glib::RefPtr<Gtk::TextBuffer::Tag> ptag = Gtk::TextBuffer::Tag::create();
	Glib::RefPtr<Gtk::TextBuffer::Tag> mtag = Gtk::TextBuffer::Tag::create();

	Pango::FontDescription pfont(style->get_font());
	ptag->property_font_desc().set_value(pfont);
	ptag->property_foreground_gdk().set_value(style->get_fg(Gtk::STATE_ACTIVE));
	ptag->property_background_gdk().set_value(style->get_bg(Gtk::STATE_ACTIVE));

	Pango::FontDescription mfont (style->get_font());
	mtag->property_font_desc().set_value(mfont);
	mtag->property_foreground_gdk().set_value(style->get_fg(Gtk::STATE_NORMAL));
	mtag->property_background_gdk().set_value(style->get_bg(Gtk::STATE_NORMAL));

	Glib::RefPtr<Gtk::TextBuffer> buffer (errors->text().get_buffer());
	buffer->insert_with_tag(buffer->end(), prefix, ptag);
	buffer->insert_with_tag(buffer->end(), msg, mtag);
	buffer->insert_with_tag(buffer->end(), "\n", mtag);

	errors->scroll_to_bottom ();
}	

void
UI::handle_fatal (const char *message)
{
	Gtk::Window win (Gtk::WINDOW_POPUP);
	Gtk::VBox packer;
	Gtk::Label label (message);
	Gtk::Button quit (_("Press To Exit"));

	win.set_default_size (400, 100);
	
	string title;
	title = _ui_name;
	title += ": Fatal Error";
	win.set_title (title);

	win.set_position (Gtk::WIN_POS_MOUSE);
	win.add (packer);

	packer.pack_start (label, true, true);
	packer.pack_start (quit, false, false);
	quit.signal_clicked().connect(mem_fun(*this,&UI::quit));
	
	win.show_all ();
	win.set_modal (true);

	theMain->run ();
	
	exit (1);
}

void
UI::popup_error (const char *text)
{
	PopUp *pup;

	if (!caller_is_gui_thread()) {
		error << "non-UI threads can't use UI::popup_error" 
		      << endmsg;
		return;
	}
	
	pup = new PopUp (Gtk::WIN_POS_MOUSE, 0, true);
	pup->set_text (text);
	pup->touch ();
}


void
UI::flush_pending ()
{
	if (!caller_is_gui_thread()) {
		error << "non-UI threads cannot call UI::flush_pending()"
		      << endmsg;
		return;
	}

	gtk_main_iteration();

	while (gtk_events_pending()) {
		gtk_main_iteration();
	}
}

bool
UI::just_hide_it (GdkEventAny *ev, Gtk::Window *win)
{
	win->hide_all ();
	return true;
}

Gdk::Color
UI::get_color (const string& prompt, bool& picked, Gdk::Color* initial)
{
	Gdk::Color color;

	Gtk::ColorSelectionDialog color_dialog (prompt);

	color_dialog.set_modal (true);
	color_dialog.get_cancel_button()->signal_clicked().connect (bind (mem_fun (*this, &UI::color_selection_done), false));
	color_dialog.get_ok_button()->signal_clicked().connect (bind (mem_fun (*this, &UI::color_selection_done), true));
	color_dialog.signal_delete_event().connect (mem_fun (*this, &UI::color_selection_deleted));

	if (initial) {
		color_dialog.get_colorsel()->set_current_color (*initial);
	}

	color_dialog.show_all ();
	color_picked = false;
	picked = false;

	Gtk::Main::run();

	color_dialog.hide_all ();

	if (color_picked) {
		Gdk::Color f_rgba = color_dialog.get_colorsel()->get_current_color ();
		color.set_red(f_rgba.get_red());
		color.set_green(f_rgba.get_green());
		color.set_blue(f_rgba.get_blue());

		picked = true;
	}

	return color;
}

void
UI::color_selection_done (bool status)
{
	color_picked = status;
	Gtk::Main::quit ();
}

bool
UI::color_selection_deleted (GdkEventAny *ev)
{
	Gtk::Main::quit ();
	return true;
}
