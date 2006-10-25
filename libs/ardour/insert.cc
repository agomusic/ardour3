/*
    Copyright (C) 2000 Paul Davis 

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

#include <sigc++/bind.h>

#include <pbd/failed_constructor.h>
#include <pbd/xml++.h>

#include <ardour/insert.h>
#include <ardour/plugin.h>
#include <ardour/port.h>
#include <ardour/route.h>
#include <ardour/ladspa_plugin.h>

#ifdef VST_SUPPORT
#include <ardour/vst_plugin.h>
#endif

#ifdef HAVE_AUDIOUNITS
#include <ardour/audio_unit.h>
#endif

#include <ardour/audioengine.h>
#include <ardour/session.h>
#include <ardour/types.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

Insert::Insert(Session& s, Placement p)
	: Redirect (s, s.next_insert_name(), p)
{
}

Insert::Insert(Session& s, Placement p, int imin, int imax, int omin, int omax)
	: Redirect (s, s.next_insert_name(), p, imin, imax, omin, omax)
{
}

Insert::Insert(Session& s, string name, Placement p)
	: Redirect (s, name, p)
{
}

/***************************************************************
 Plugin inserts: send data through a plugin
 ***************************************************************/

const string PluginInsert::port_automation_node_name = "PortAutomation";

PluginInsert::PluginInsert (Session& s, boost::shared_ptr<Plugin> plug, Placement placement)
	: Insert (s, plug->name(), placement)
{
	/* the first is the master */

	_plugins.push_back (plug);

	_plugins[0]->ParameterChanged.connect (mem_fun (*this, &PluginInsert::parameter_changed));
	
	init ();

#ifdef STATE_MANAGER
	save_state (_("initial state"));
#endif

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		IO::MoreOutputs (output_streams ());
	}

	RedirectCreated (this); /* EMIT SIGNAL */
}

PluginInsert::PluginInsert (Session& s, const XMLNode& node)
	: Insert (s, "will change", PreFader)
{
	if (set_state (node)) {
		throw failed_constructor();
	}

	set_automatable ();

#ifdef STATE_MANAGER
	save_state (_("initial state"));
#endif

	_plugins[0]->ParameterChanged.connect (mem_fun (*this, &PluginInsert::parameter_changed));

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		IO::MoreOutputs (output_streams());
	}
}

PluginInsert::PluginInsert (const PluginInsert& other)
	: Insert (other._session, other.plugin()->name(), other.placement())
{
	uint32_t count = other._plugins.size();

	/* make as many copies as requested */
	for (uint32_t n = 0; n < count; ++n) {
		_plugins.push_back (plugin_factory (other.plugin (n)));
	}


	_plugins[0]->ParameterChanged.connect (mem_fun (*this, &PluginInsert::parameter_changed));

	init ();

#ifdef STATE_MANAGER
	save_state (_("initial state"));
#endif

	RedirectCreated (this); /* EMIT SIGNAL */
}

int
PluginInsert::set_count (uint32_t num)
{
	bool require_state = !_plugins.empty();

	/* this is a bad idea.... we shouldn't do this while active.
	   only a route holding their redirect_lock should be calling this 
	*/

	if (num == 0) { 
		return -1;
	} else if (num > _plugins.size()) {
		uint32_t diff = num - _plugins.size();

		for (uint32_t n = 0; n < diff; ++n) {
			_plugins.push_back (plugin_factory (_plugins[0]));

			if (require_state) {
				/* XXX do something */
			}
		}

	} else if (num < _plugins.size()) {
		uint32_t diff = _plugins.size() - num;
		for (uint32_t n= 0; n < diff; ++n) {
			_plugins.pop_back();
		}
	}

	return 0;
}

void
PluginInsert::init ()
{
	set_automatable ();

	set<uint32_t>::iterator s;
}

PluginInsert::~PluginInsert ()
{
	GoingAway (); /* EMIT SIGNAL */
}

void
PluginInsert::automation_list_creation_callback (uint32_t which, AutomationList& alist)
{
  alist.automation_state_changed.connect (sigc::bind (mem_fun (*this, &PluginInsert::auto_state_changed), (which)));
}

void
PluginInsert::auto_state_changed (uint32_t which)
{
	AutomationList& alist (automation_list (which));

	if (alist.automation_state() != Off) {
		_plugins[0]->set_parameter (which, alist.eval (_session.transport_frame()));
	}
}

uint32_t
PluginInsert::output_streams() const
{
	return _plugins[0]->get_info()->n_outputs * _plugins.size();
}

uint32_t
PluginInsert::input_streams() const
{
	return _plugins[0]->get_info()->n_inputs * _plugins.size();
}

uint32_t
PluginInsert::natural_output_streams() const
{
	return _plugins[0]->get_info()->n_outputs;
}

uint32_t
PluginInsert::natural_input_streams() const
{
	return _plugins[0]->get_info()->n_inputs;
}

bool
PluginInsert::is_generator() const
{
	/* XXX more finesse is possible here. VST plugins have a
	   a specific "instrument" flag, for example.
	 */

	return _plugins[0]->get_info()->n_inputs == 0;
}

void
PluginInsert::set_automatable ()
{
	set<uint32_t> a;
	
	a = _plugins.front()->automatable ();

	for (set<uint32_t>::iterator i = a.begin(); i != a.end(); ++i) {
		can_automate (*i);
	}
}

void
PluginInsert::parameter_changed (uint32_t which, float val)
{
	vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin();

	/* don't set the first plugin, just all the slaves */

	if (i != _plugins.end()) {
		++i;
		for (; i != _plugins.end(); ++i) {
			(*i)->set_parameter (which, val);
		}
	}
}

void
PluginInsert::set_block_size (nframes_t nframes)
{
	for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
		(*i)->set_block_size (nframes);
	}
}

void
PluginInsert::activate ()
{
	for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
		(*i)->activate ();
	}
}

void
PluginInsert::deactivate ()
{
	for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
		(*i)->deactivate ();
	}
}

void
PluginInsert::connect_and_run (vector<Sample*>& bufs, uint32_t nbufs, nframes_t nframes, nframes_t offset, bool with_auto, nframes_t now)
{
	int32_t in_index = 0;
	int32_t out_index = 0;

	/* Note that we've already required that plugins
	   be able to handle in-place processing.
	*/

	if (with_auto) {

		map<uint32_t,AutomationList*>::iterator li;
		uint32_t n;
		
		for (n = 0, li = parameter_automation.begin(); li != parameter_automation.end(); ++li, ++n) {
			
			AutomationList& alist (*((*li).second));

			if (alist.automation_playback()) {
				bool valid;

				float val = alist.rt_safe_eval (now, valid);				

				if (valid) {
					/* set the first plugin, the others will be set via signals */
					_plugins[0]->set_parameter ((*li).first, val);
				}

			} 
		}
	}

	for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
		(*i)->connect_and_run (bufs, nbufs, in_index, out_index, nframes, offset);
	}

	/* leave remaining channel buffers alone */
}

void
PluginInsert::transport_stopped (nframes_t now)
{
	map<uint32_t,AutomationList*>::iterator li;

	for (li = parameter_automation.begin(); li != parameter_automation.end(); ++li) {
		AutomationList& alist (*(li->second));
		alist.reposition_for_rt_add (now);

		if (alist.automation_state() != Off) {
			_plugins[0]->set_parameter (li->first, alist.eval (now));
		}
	}
}

void
PluginInsert::silence (nframes_t nframes, nframes_t offset)
{
	int32_t in_index = 0;
	int32_t out_index = 0;

	uint32_t n;

	if (active()) {
		for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
			n = (*i) -> get_info()->n_inputs;
			(*i)->connect_and_run (_session.get_silent_buffers (n), n, in_index, out_index, nframes, offset);
		}
	}
}
	
void
PluginInsert::run (vector<Sample *>& bufs, uint32_t nbufs, nframes_t nframes, nframes_t offset)
{
	if (active()) {

		if (_session.transport_rolling()) {
			automation_run (bufs, nbufs, nframes, offset);
		} else {
			connect_and_run (bufs, nbufs, nframes, offset, false);
		}
	} else {
		uint32_t in = _plugins[0]->get_info()->n_inputs;
		uint32_t out = _plugins[0]->get_info()->n_outputs;

		if (out > in) {

			/* not active, but something has make up for any channel count increase */
			
			for (uint32_t n = out - in; n < out; ++n) {
				memcpy (bufs[n], bufs[in - 1], sizeof (Sample) * nframes);
			}
		}
	}
}

void
PluginInsert::set_parameter (uint32_t port, float val)
{
	/* the others will be set from the event triggered by this */

	_plugins[0]->set_parameter (port, val);
	
	if (automation_list (port).automation_write()) {
		automation_list (port).add (_session.audible_frame(), val);
	}

	_session.set_dirty();
}

void
PluginInsert::automation_run (vector<Sample *>& bufs, uint32_t nbufs, nframes_t nframes, nframes_t offset)
{
	ControlEvent next_event (0, 0.0f);
	nframes_t now = _session.transport_frame ();
	nframes_t end = now + nframes;

	Glib::Mutex::Lock lm (_automation_lock, Glib::TRY_LOCK);

	if (!lm.locked()) {
		connect_and_run (bufs, nbufs, nframes, offset, false);
		return;
	}
	
	if (!find_next_event (now, end, next_event)) {
		
 		/* no events have a time within the relevant range */
		
 		connect_and_run (bufs, nbufs, nframes, offset, true, now);
 		return;
 	}
	
 	while (nframes) {

 		nframes_t cnt = min (((nframes_t) floor (next_event.when) - now), nframes);
  
 		connect_and_run (bufs, nbufs, cnt, offset, true, now);
 		
 		nframes -= cnt;
 		offset += cnt;
		now += cnt;

		if (!find_next_event (now, end, next_event)) {
			break;
		}
  	}
  
 	/* cleanup anything that is left to do */
  
 	if (nframes) {
 		connect_and_run (bufs, nbufs, nframes, offset, true, now);
  	}
}	

float
PluginInsert::default_parameter_value (uint32_t port)
{
	if (_plugins.empty()) {
		fatal << _("programming error: ") << X_("PluginInsert::default_parameter_value() called with no plugin")
		      << endmsg;
		/*NOTREACHED*/
	}

	return _plugins[0]->default_value (port);
}
	
void
PluginInsert::set_port_automation_state (uint32_t port, AutoState s)
{
	if (port < _plugins[0]->parameter_count()) {
		
		AutomationList& al = automation_list (port);

		if (s != al.automation_state()) {
			al.set_automation_state (s);
			_session.set_dirty ();
		}
	}
}

AutoState
PluginInsert::get_port_automation_state (uint32_t port)
{
	if (port < _plugins[0]->parameter_count()) {
		return automation_list (port).automation_state();
	} else {
		return Off;
	}
}

void
PluginInsert::protect_automation ()
{
	set<uint32_t> automated_params;

	what_has_automation (automated_params);

	for (set<uint32_t>::iterator i = automated_params.begin(); i != automated_params.end(); ++i) {

		AutomationList& al = automation_list (*i);

		switch (al.automation_state()) {
		case Write:
		case Touch:
			al.set_automation_state (Off);
			break;
		default:
			break;
		}
	}
}

boost::shared_ptr<Plugin>
PluginInsert::plugin_factory (boost::shared_ptr<Plugin> other)
{
	boost::shared_ptr<LadspaPlugin> lp;
#ifdef VST_SUPPORT
	boost::shared_ptr<VSTPlugin> vp;
#endif
#ifdef HAVE_AUDIOUNITS
	boost::shared_ptr<AUPlugin> ap;
#endif

	if ((lp = boost::dynamic_pointer_cast<LadspaPlugin> (other)) != 0) {
		return boost::shared_ptr<Plugin> (new LadspaPlugin (*lp));
#ifdef VST_SUPPORT
	} else if ((vp = boost::dynamic_pointer_cast<VSTPlugin> (other)) != 0) {
		return boost::shared_ptr<Plugin> (new VSTPlugin (*vp));
#endif
#ifdef HAVE_AUDIOUNITS
	} else if ((ap = boost::dynamic_pointer_cast<AUPlugin> (other)) != 0) {
		return boost::shared_ptr<Plugin> (new AUPlugin (*ap));
#endif
	}

	fatal << string_compose (_("programming error: %1"),
			  X_("unknown plugin type in PluginInsert::plugin_factory"))
	      << endmsg;
	/*NOTREACHED*/
	return boost::shared_ptr<Plugin> ((Plugin*) 0);
}

int32_t
PluginInsert::compute_output_streams (int32_t cnt) const
{
	return _plugins[0]->get_info()->n_outputs * cnt;
}

int32_t
PluginInsert::configure_io (int32_t magic, int32_t in, int32_t out)
{
	return set_count (magic);
}

int32_t 
PluginInsert::can_support_input_configuration (int32_t in) const
{
	int32_t outputs = _plugins[0]->get_info()->n_outputs;
	int32_t inputs = _plugins[0]->get_info()->n_inputs;

	if (inputs == 0) {

		/* instrument plugin, always legal, but it throws
		   away any existing active streams.
		*/

		return 1;
	}

	if (outputs == 1 && inputs == 1) {
		/* mono plugin, replicate as needed */
		return in;
	}

	if (inputs == in) {
		/* exact match */
		return 1;
	}

	if ((inputs < in) && (inputs % in == 0)) {

		/* number of inputs is a factor of the requested input
		   configuration, so we can replicate.
		*/

		return in/inputs;
	}

	/* sorry */

	return -1;
}

XMLNode&
PluginInsert::get_state(void)
{
	return state (true);
}

XMLNode&
PluginInsert::state (bool full)
{
	char buf[256];
	XMLNode *node = new XMLNode("Insert");

	node->add_child_nocopy (Redirect::state (full));

	node->add_property ("type", _plugins[0]->state_node_name());
	snprintf(buf, sizeof(buf), "%s", _plugins[0]->name());
	node->add_property("id", string(buf));
	if (_plugins[0]->state_node_name() == "ladspa") {
		char buf[32];
		snprintf (buf, sizeof (buf), "%ld", _plugins[0]->get_info()->unique_id); 
		node->add_property("unique-id", string(buf));
	}
	node->add_property("count", string_compose("%1", _plugins.size()));
	node->add_child_nocopy (_plugins[0]->get_state());

	/* add port automation state */
	XMLNode *autonode = new XMLNode(port_automation_node_name);
	set<uint32_t> automatable = _plugins[0]->automatable();

	for (set<uint32_t>::iterator x =  automatable.begin(); x != automatable.end(); ++x) {

		XMLNode* child = new XMLNode("port");
		snprintf(buf, sizeof(buf), "%" PRIu32, *x);
		child->add_property("number", string(buf));
		
		if (full) {
			snprintf(buf, sizeof(buf), "0x%x", automation_list (*x).automation_state ());
		} else {
			snprintf(buf, sizeof(buf), "0x%x", ARDOUR::Off);
		}
		child->add_property("auto", string(buf));
		
		autonode->add_child_nocopy (*child);
	}

	node->add_child_nocopy (*autonode);
	
	return *node;
}

int
PluginInsert::set_state(const XMLNode& node)
{
	XMLNodeList nlist = node.children();
	XMLNodeIterator niter;
	XMLPropertyList plist;
	const XMLProperty *prop;
	long unique = 0;
	ARDOUR::PluginType type;

	if ((prop = node.property ("type")) == 0) {
		error << _("XML node describing insert is missing the `type' field") << endmsg;
		return -1;
	}

	if (prop->value() == X_("ladspa") || prop->value() == X_("Ladspa")) { /* handle old school sessions */
		type = ARDOUR::LADSPA;
	} else if (prop->value() == X_("vst")) {
		type = ARDOUR::VST;
	} else {
		error << string_compose (_("unknown plugin type %1 in plugin insert state"),
				  prop->value())
		      << endmsg;
		return -1;
	}

	prop = node.property ("unique-id");
	if (prop != 0) {
		unique = atol(prop->value().c_str());
	}

	if ((prop = node.property ("id")) == 0) {
		error << _("XML node describing insert is missing the `id' field") << endmsg;
 		return -1;
	}

	boost::shared_ptr<Plugin> plugin;
	
	if (unique != 0) {
		plugin = find_plugin (_session, "", unique, type);	
	} else {
		plugin = find_plugin (_session, prop->value(), 0, type);	
	}

	if (plugin == 0) {
		error << string_compose(_("Found a reference to a plugin (\"%1\") that is unknown.\n"
				   "Perhaps it was removed or moved since it was last used."), prop->value()) 
		      << endmsg;
		return -1;
	}

	uint32_t count = 1;

	if ((prop = node.property ("count")) != 0) {
		sscanf (prop->value().c_str(), "%u", &count);
	}

	if (_plugins.size() != count) {
		
		_plugins.push_back (plugin);
		
		for (uint32_t n=1; n < count; ++n) {
			_plugins.push_back (plugin_factory (plugin));
		}
	}
	
	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		if ((*niter)->name() == plugin->state_node_name()) {
			for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
				(*i)->set_state (**niter);
			}
			break;
		}
	} 

	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		if ((*niter)->name() == Redirect::state_node_name) {
			Redirect::set_state (**niter);
			break;
		}
	}

	if (niter == nlist.end()) {
		error << _("XML node describing insert is missing a Redirect node") << endmsg;
		return -1;
	}

	if (niter == nlist.end()) {
		error << string_compose(_("XML node describing a plugin insert is missing the `%1' information"), plugin->state_node_name()) << endmsg;
		return -1;
	}
	
	/* look for port automation node */
	
	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		if ((*niter)->name() == port_automation_node_name) {
			XMLNodeList cnodes;
			XMLProperty *cprop;
			XMLNodeConstIterator iter;
			XMLNode *child;
			const char *port;
			uint32_t port_id;

			cnodes = (*niter)->children ("port");
	
			for(iter = cnodes.begin(); iter != cnodes.end(); ++iter){
				
				child = *iter;
				
				if ((cprop = child->property("number")) != 0) {
					port = cprop->value().c_str();
				} else {
					warning << _("PluginInsert: Auto: no ladspa port number") << endmsg;
					continue;
				}

				sscanf (port, "%" PRIu32, &port_id);

				if (port_id >= _plugins[0]->parameter_count()) {
					warning << _("PluginInsert: Auto: port id out of range") << endmsg;
					continue;
				}
				
				if ((cprop = child->property("auto")) != 0) {
					int x;
					sscanf (cprop->value().c_str(), "0x%x", &x);
					automation_list (port_id).set_automation_state (AutoState (x));
				}
			}
			
			break;
		}
	} 

	if (niter == nlist.end()) {
		warning << string_compose(_("XML node describing a port automation is missing the `%1' information"), port_automation_node_name) << endmsg;
	}
	
	// The name of the PluginInsert comes from the plugin, nothing else
	set_name(plugin->get_info()->name,this);
	
	return 0;
}

string
PluginInsert::describe_parameter (uint32_t what)
{
	return _plugins[0]->describe_parameter (what);
}

nframes_t 
PluginInsert::latency() 
{
	return _plugins[0]->latency ();
}
	
void
PluginInsert::store_state (PluginInsertState& state) const
{
	Redirect::store_state (state);
	_plugins[0]->store_state (state.plugin_state);
}

Change
PluginInsert::restore_state (StateManager::State& state)
{
	PluginInsertState* pistate = dynamic_cast<PluginInsertState*> (&state);

	Redirect::restore_state (state);

	_plugins[0]->restore_state (pistate->plugin_state);

	return Change (0);
}

StateManager::State*
PluginInsert::state_factory (std::string why) const
{
	PluginInsertState* state = new PluginInsertState (why);

	store_state (*state);

	return state;
}

ARDOUR::PluginType
PluginInsert::type ()
{
	boost::shared_ptr<LadspaPlugin> lp;
#ifdef VST_SUPPORT
	boost::shared_ptr<VSTPlugin> vp;
#endif
#ifdef HAVE_AUDIOUNITS
	boost::shared_ptr<AUPlugin> ap;
#endif
	
	PluginPtr other = plugin ();

	if ((lp = boost::dynamic_pointer_cast<LadspaPlugin> (other)) != 0) {
		return ARDOUR::LADSPA;
#ifdef VST_SUPPORT
	} else if ((vp = boost::dynamic_pointer_cast<VSTPlugin> (other)) != 0) {
		return ARDOUR::VST;
#endif
#ifdef HAVE_AUDIOUNITS
	} else if ((ap = boost::dynamic_pointer_cast<AUPlugin> (other)) != 0) {
		return ARDOUR::AudioUnit;
#endif
	} else {
		/* NOT REACHED */
		return (ARDOUR::PluginType) 0;
	}
}

/***************************************************************
 Port inserts: send output to a port, pick up input at a port
 ***************************************************************/

PortInsert::PortInsert (Session& s, Placement p)
	: Insert (s, p, 1, -1, 1, -1)
{
	init ();
#ifdef STATE_MANAGER
	save_state (_("initial state"));
	RedirectCreated (this); /* EMIT SIGNAL */
#endif

}

PortInsert::PortInsert (const PortInsert& other)
	: Insert (other._session, other.placement(), 1, -1, 1, -1)
{
	init ();
#ifdef STATE_MANAGER
	save_state (_("initial state"));
	RedirectCreated (this); /* EMIT SIGNAL */
#endif

}

void
PortInsert::init ()
{
	if (add_input_port ("", this)) {
		error << _("PortInsert: cannot add input port") << endmsg;
		throw failed_constructor();
	}
	
	if (add_output_port ("", this)) {
		error << _("PortInsert: cannot add output port") << endmsg;
		throw failed_constructor();
	}
}

PortInsert::PortInsert (Session& s, const XMLNode& node)
	: Insert (s, "will change", PreFader)
{
	if (set_state (node)) {
		throw failed_constructor();
	}

	RedirectCreated (this); /* EMIT SIGNAL */
}

PortInsert::~PortInsert ()
{
	GoingAway ();
}

void
PortInsert::run (vector<Sample *>& bufs, uint32_t nbufs, nframes_t nframes, nframes_t offset)
{
	if (n_outputs() == 0) {
		return;
	}

	if (!active()) {
		/* deliver silence */
		silence (nframes, offset);
		return;
	}

	uint32_t n;
	vector<Port*>::iterator o;
	vector<Port*>::iterator i;

	/* deliver output */

	for (o = _outputs.begin(), n = 0; o != _outputs.end(); ++o, ++n) {
		memcpy ((*o)->get_buffer (nframes) + offset, bufs[min(nbufs,n)], sizeof (Sample) * nframes);
		(*o)->mark_silence (false);
	}
	
	/* collect input */
	
	for (i = _inputs.begin(), n = 0; i != _inputs.end(); ++i, ++n) {
		memcpy (bufs[min(nbufs,n)], (*i)->get_buffer (nframes) + offset, sizeof (Sample) * nframes);
	}
}

XMLNode&
PortInsert::get_state(void)
{
	return state (true);
}

XMLNode&
PortInsert::state (bool full)
{
	XMLNode *node = new XMLNode("Insert");

	node->add_child_nocopy (Redirect::state(full));	
	node->add_property("type", "port");

	return *node;
}

int
PortInsert::set_state(const XMLNode& node)
{
	XMLNodeList nlist = node.children();
	XMLNodeIterator niter;
	XMLPropertyList plist;
	const XMLProperty *prop;

	if ((prop = node.property ("type")) == 0) {
		error << _("XML node describing insert is missing the `type' field") << endmsg;
		return -1;
	}
	
	if (prop->value() != "port") {
		error << _("non-port insert XML used for port plugin insert") << endmsg;
		return -1;
	}

	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {
		if ((*niter)->name() == Redirect::state_node_name) {
			Redirect::set_state (**niter);
			break;
		}
	}

	if (niter == nlist.end()) {
		error << _("XML node describing insert is missing a Redirect node") << endmsg;
		return -1;
	}

	return 0;
}

nframes_t 
PortInsert::latency() 
{
	/* because we deliver and collect within the same cycle,
	   all I/O is necessarily delayed by at least frames_per_cycle().

	   if the return port for insert has its own latency, we
	   need to take that into account too.
	*/

	return _session.engine().frames_per_cycle() + input_latency();
}

int32_t
PortInsert::can_support_input_configuration (int32_t in) const
{
	if (input_maximum() == -1 && output_maximum() == -1) {

		/* not configured yet */

		return 1; /* we can support anything the first time we're asked */

	} else {

		/* the "input" config for a port insert corresponds to how
		   many output ports it will have.
		*/

		if (output_maximum() == in) {
			return 1;
		} 
	}

	return -1;
}

int32_t
PortInsert::configure_io (int32_t ignored_magic, int32_t in, int32_t out)
{
	/* do not allow configuration to be changed outside the range of
	   the last request config. or something like that.
	*/


	/* this is a bit odd: 

	   the number of inputs we are required to handle corresponds 
	   to the number of output ports we need.

	   the number of outputs we are required to have corresponds
	   to the number of input ports we need.
	*/

	set_output_maximum (in);
	set_output_minimum (in);
	set_input_maximum (out);
	set_input_minimum (out);

	if (in < 0) {
		in = n_outputs ();
	} 

	if (out < 0) {
		out = n_inputs ();
	}

	return ensure_io (out, in, false, this);
}

int32_t
PortInsert::compute_output_streams (int32_t cnt) const
{
	/* puzzling, eh? think about it ... */
	return n_inputs ();
}

uint32_t
PortInsert::output_streams() const
{
	return n_inputs ();
}

uint32_t
PortInsert::input_streams() const
{
	return n_outputs ();
}

