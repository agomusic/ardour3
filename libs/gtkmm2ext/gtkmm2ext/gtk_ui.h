/*
    Copyright (C) 1999 Paul Barton-Davis 

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

#ifndef __pbd_gtk_ui_h__
#define __pbd_gtk_ui_h__

#include <string>
#include <queue>
#include <map>

#include <stdint.h>
#include <setjmp.h>
#include <pthread.h>
#include <gtkmm/widget.h>
#include <gtkmm/style.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/main.h>
#include <gtkmm/tooltips.h>
#include <gdkmm/color.h>
#include <pbd/abstract_ui.h>
#include <pbd/ringbufferNPT.h>
 
#include <pbd/pool.h>
#include <pbd/error.h>
#include <pbd/receiver.h>

using std::string;
using std::queue;

class Touchable;

namespace Gtkmm2ext {

class TextViewer;

extern BaseUI::RequestType ErrorMessage;
extern BaseUI::RequestType Quit;
extern BaseUI::RequestType CallSlot;
extern BaseUI::RequestType TouchDisplay;
extern BaseUI::RequestType StateChange;
extern BaseUI::RequestType SetTip;
extern BaseUI::RequestType AddIdle;
extern BaseUI::RequestType AddTimeout;

struct UIRequest : public BaseUI::BaseRequestObject {
     
     /* this once used anonymous unions to merge elements
	that are never part of the same request. that makes
	the creation of a legal copy constructor difficult
	because of the semantics of the slot member.
     */
     
    Touchable *display;
    const char *msg;
    Gtk::StateType new_state;
    int (*function)(void *);
    Gtk::Widget *widget;
    Transmitter::Channel chn;
    void *arg;
    const char *msg2;
    sigc::slot<void> slot;
    
    ~UIRequest () { 
	    if (type == ErrorMessage && msg) {
		    /* msg was strdup()'ed */
		    free ((char *)msg);
	    }
    }
 };

class UI : public Receiver, public AbstractUI<UIRequest>
{
  public:
	UI (string name, int *argc, char **argv[]);
	virtual ~UI ();

	static UI *instance() { return theGtkUI; }

	/* receiver interface */

	void receive (Transmitter::Channel, const char *);

	/* Abstract UI interfaces */

	bool caller_is_ui_thread ();

	static pthread_t thread_id() { return gui_thread; }

	/* Gtk-UI specific interfaces */

	bool running ();
	void quit    ();
	void kill    ();
	int  load_rcfile (string);
	void run (Receiver &old_receiver);

	void set_state (Gtk::Widget *w, Gtk::StateType state);
	void popup_error (const char *text);
	void flush_pending ();
	void toggle_errors ();
	void touch_display (Touchable *);
	void set_tip (Gtk::Widget *w, const gchar *tip, const gchar *hlp);
	void idle_add (int (*func)(void *), void *arg);

	template<class T> static bool idle_delete (T *obj) { delete obj; return false; }
	template<class T> static void delete_when_idle (T *obj) {
		Glib::signal_idle().connect (bind (slot (&UI::idle_delete<T>), obj));
	}

	Gdk::Color get_color (const string& prompt, bool& picked, const Gdk::Color *initial = 0);

	/* starting is sent just before we enter the main loop,
	   stopping just after we return from it (at the top level)
	*/

	sigc::signal<void> starting;
	sigc::signal<void> stopping;

	sigc::signal<void> theme_changed;

	static bool just_hide_it (GdkEventAny *, Gtk::Window *);

	static pthread_t the_gui_thread() { return gui_thread; }

  protected:
	virtual void handle_fatal (const char *);
	virtual void display_message (const char *prefix, gint prefix_len, 
				      Glib::RefPtr<Gtk::TextBuffer::Tag> ptag, 
				      Glib::RefPtr<Gtk::TextBuffer::Tag> mtag, 
				      const char *msg);

  private:
	static UI *theGtkUI;
	static pthread_t gui_thread;
	bool _active;
	Gtk::Main *theMain;
	Gtk::Tooltips *tips;
	TextViewer *errors;
	Glib::RefPtr<Gtk::TextBuffer::Tag> error_ptag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> error_mtag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> fatal_ptag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> fatal_mtag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> info_ptag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> info_mtag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> warning_ptag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> warning_mtag;

	static void signal_pipe_callback (void *, gint, GdkInputCondition);
	void process_error_message (Transmitter::Channel, const char *);
	void do_quit ();

	void color_selection_done (bool status);
	bool color_selection_deleted (GdkEventAny *);
	bool color_picked;

	void do_request (UIRequest*);
};

} /* namespace */

#endif /* __pbd_gtk_ui_h__ */
