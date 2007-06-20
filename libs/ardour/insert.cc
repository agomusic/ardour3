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
#include <ardour/buffer_set.h>

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

/* ********** FIXME: TYPE **************
 * Inserts are still definitely audio only */
Insert::Insert(Session& s, string name, Placement p)
	: Redirect (s, name, p)
	, _configured(false)
{
}

Insert::Insert(Session& s, string name, Placement p, int imin, int imax, int omin, int omax)
	: Redirect (s, name, p, imin, imax, omin, omax)
	, _configured(false)
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

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		IO::MoreChannels (max(input_streams(), output_streams()));
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

	_plugins[0]->ParameterChanged.connect (mem_fun (*this, &PluginInsert::parameter_changed));

	{
		Glib::Mutex::Lock em (_session.engine().process_lock());
		IO::MoreChannels (max(input_streams(), output_streams()));
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

	RedirectCreated (this); /* EMIT SIGNAL */
}

bool
PluginInsert::set_count (uint32_t num)
{
	bool require_state = !_plugins.empty();

	/* this is a bad idea.... we shouldn't do this while active.
	   only a route holding their redirect_lock should be calling this 
	*/

	if (num == 0) { 
		return false;
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

	return true;
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

ChanCount
PluginInsert::output_streams() const
{
	if (_configured)
		return output_for_input_configuration(_configured_input);
	else
		return natural_output_streams();
}

ChanCount
PluginInsert::input_streams() const
{
	if (_configured)
		return _configured_input;
	else
		return natural_input_streams();
}

ChanCount
PluginInsert::natural_output_streams() const
{
	return _plugins[0]->get_info()->n_outputs;
}

ChanCount
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

	return _plugins[0]->get_info()->n_inputs.n_audio() == 0;
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
PluginInsert::connect_and_run (BufferSet& bufs, nframes_t nframes, nframes_t offset, bool with_auto, nframes_t now)
{
	uint32_t in_index = 0;
	uint32_t out_index = 0;

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
		(*i)->connect_and_run (bufs, in_index, out_index, nframes, offset);
	}

	/* leave remaining channel buffers alone */
}

void
PluginInsert::automation_snapshot (nframes_t now)
{
	map<uint32_t,AutomationList*>::iterator li;
	
	for (li = parameter_automation.begin(); li != parameter_automation.end(); ++li) {
		
		AutomationList *alist = ((*li).second);
		if (alist != 0 && alist->automation_write ()) {
			
			float val = _plugins[0]->get_parameter ((*li).first);
			alist->rt_add (now, val);
			last_automation_snapshot = now;
		}
	}
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
	uint32_t in_index = 0;
	uint32_t out_index = 0;

	if (active()) {
		for (vector<boost::shared_ptr<Plugin> >::iterator i = _plugins.begin(); i != _plugins.end(); ++i) {
			(*i)->connect_and_run (_session.get_silent_buffers ((*i)->get_info()->n_inputs), in_index, out_index, nframes, offset);
		}
	}
}
	
void
PluginInsert::run (BufferSet& bufs, nframes_t start_frame, nframes_t end_frame, nframes_t nframes, nframes_t offset)
{
	if (active()) {

		if (_session.transport_rolling()) {
			automation_run (bufs, nframes, offset);
		} else {
			connect_and_run (bufs, nframes, offset, false);
		}
	} else {
		uint32_t in = _plugins[0]->get_info()->n_inputs.n_audio();
		uint32_t out = _plugins[0]->get_info()->n_outputs.n_audio();

		if (out > in) {

			/* not active, but something has make up for any channel count increase */
			
			for (uint32_t n = out - in; n < out; ++n) {
				memcpy (bufs.get_audio(n).data(nframes, offset), bufs.get_audio(in - 1).data(nframes, offset), sizeof (Sample) * nframes);
			}
		}

		bufs.count().set(_default_type, out);
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
PluginInsert::automation_run (BufferSet& bufs, nframes_t nframes, nframes_t offset)
{
	ControlEvent next_event (0, 0.0f);
	nframes_t now = _session.transport_frame ();
	nframes_t end = now + nframes;

	Glib::Mutex::Lock lm (_automation_lock, Glib::TRY_LOCK);

	if (!lm.locked()) {
		connect_and_run (bufs, nframes, offset, false);
		return;
	}
	
	if (!find_next_event (now, end, next_event)) {
		
 		/* no events have a time within the relevant range */
		
 		connect_and_run (bufs, nframes, offset, true, now);
 		return;
 	}
	
 	while (nframes) {

		nframes_t cnt = min (((nframes_t) ceil (next_event.when) - now), nframes);
  
 		connect_and_run (bufs, cnt, offset, true, now);
 		
 		nframes -= cnt;
 		offset += cnt;
		now += cnt;

		if (!find_next_event (now, end, next_event)) {
			break;
		}
  	}
  
 	/* cleanup anything that is left to do */
  
 	if (nframes) {
 		connect_and_run (bufs, nframes, offset, true, now);
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
			al.set_automation_state (Off);
			break;
		case Touch:
			al.set_automation_state (Play);
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

bool
PluginInsert::configure_io (ChanCount in, ChanCount out)
{
	ChanCount matching_out = output_for_input_configuration(out);
	if (matching_out != out) {
		_configured = false;
		return false;
	} else {
		bool success = set_count (count_for_configuration(in, out));
		if (success) {
			_configured = true;
			_configured_input = in;
		}
		return success;
	}
}

bool
PluginInsert::can_support_input_configuration (ChanCount in_count) const
{
	int32_t outputs = _plugins[0]->get_info()->n_outputs.get(_default_type);
	int32_t inputs = _plugins[0]->get_info()->n_inputs.get(_default_type);
	int32_t in = in_count.get(_default_type);

	/* see output_for_input_configuration below */
	if ((inputs == 0)
			|| (outputs == 1 && inputs == 1)
			|| (inputs == in)
			|| ((inputs < in) && (inputs % in == 0))) {
		return true;
	} else {
		return false;
	}
}

ChanCount
PluginInsert::output_for_input_configuration (ChanCount in) const
{
	ChanCount outputs = _plugins[0]->get_info()->n_outputs;
	ChanCount inputs = _plugins[0]->get_info()->n_inputs;

	if (inputs.n_total() == 0) {
		/* instrument plugin, always legal, but throws away any existing streams */
		return outputs;
	}

	if (inputs.n_total() == 1 && outputs == inputs) {
		/* mono plugin, replicate as needed to match in */
		return in;
	}

	if (inputs == in) {
		/* exact match */
		return outputs;
	}

	// FIXME: single type plugins only.  can we do this for instruments?
	if ((inputs.n_total() == inputs.get(_default_type))
			&& ((in.n_total() == in.get(_default_type))
			&& (inputs.n_total() < in.n_total())
			&& (inputs.n_total() % in.n_total() == 0))) {

		/* number of inputs is a factor of the requested input
		   configuration, so we can replicate.
		*/

		return ChanCount(_default_type, in.n_total() / inputs.n_total());
	}

	/* sorry */
	return ChanCount();
}

/* Number of plugin instances required to support a given channel configuration.
 * (private helper)
 */
int32_t
PluginInsert::count_for_configuration (ChanCount in, ChanCount out) const
{
	// FIXME: take 'out' into consideration
	
	ChanCount outputs = _plugins[0]->get_info()->n_outputs;
	ChanCount inputs = _plugins[0]->get_info()->n_inputs;

	if (inputs.n_total() == 0) {
		/* instrument plugin, always legal, but throws away any existing streams */
		return 1;
	}

	if (inputs.n_total() == 1 && outputs == inputs) {
		/* mono plugin, replicate as needed to match in */
		return in.n_total();
	}

	if (inputs == in) {
		/* exact match */
		return 1;
	}

	// FIXME: single type plugins only.  can we do this for instruments?
	if ((inputs.n_total() == inputs.get(_default_type))
			&& ((in.n_total() == in.get(_default_type))
			&& (inputs.n_total() < in.n_total())
			&& (inputs.n_total() % in.n_total() == 0))) {

		/* number of inputs is a factor of the requested input
		   configuration, so we can replicate.
		*/

		return in.n_total() / inputs.n_total();
	}

	/* sorry */
	return 0;
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
		
		child->add_child_nocopy (automation_list (*x).state (full));
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

		if ((*niter)->name() != port_automation_node_name) {
			continue;
		}

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

			if (!child->children().empty()) {
				automation_list (port_id).set_state (*child->children().front());
			} else {
				if ((cprop = child->property("auto")) != 0) {
					
					/* old school */

					int x;
					sscanf (cprop->value().c_str(), "0x%x", &x);
					automation_list (port_id).set_automation_state (AutoState (x));

				} else {
					
					/* missing */
					
					automation_list (port_id).set_automation_state (Off);
				}
			}

		}

		/* done */

		break;
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

ARDOUR::nframes_t 
PluginInsert::latency() 
{
	return _plugins[0]->latency ();
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
	: Insert (s, string_compose (_("insert %1"), (bitslot = s.next_insert_id()) + 1), p, 1, -1, 1, -1)
{
	init ();
	RedirectCreated (this); /* EMIT SIGNAL */
}

PortInsert::PortInsert (const PortInsert& other)
	: Insert (other._session, string_compose (_("insert %1"), (bitslot = other._session.next_insert_id()) + 1), other.placement(), 1, -1, 1, -1)
{
	init ();
	RedirectCreated (this); /* EMIT SIGNAL */
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
PortInsert::run (BufferSet& bufs, nframes_t start_frame, nframes_t end_frame, nframes_t nframes, nframes_t offset)
{
	if (n_outputs().get(_default_type) == 0) {
		return;
	}

	if (!active()) {
		/* deliver silence */
		silence (nframes, offset);
		return;
	}

	deliver_output(bufs, start_frame, end_frame, nframes, offset);

	collect_input(bufs, nframes, offset);
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
	char buf[32];
	node->add_child_nocopy (Redirect::state(full));	
	node->add_property ("type", "port");
	snprintf (buf, sizeof (buf), "%" PRIu32, bitslot);
	node->add_property ("bitslot", buf);

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

	if ((prop = node.property ("bitslot")) == 0) {
		bitslot = _session.next_insert_id();
	} else {
		sscanf (prop->value().c_str(), "%" PRIu32, &bitslot);
		_session.mark_insert_id (bitslot);
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

ARDOUR::nframes_t 
PortInsert::latency() 
{
	/* because we deliver and collect within the same cycle,
	   all I/O is necessarily delayed by at least frames_per_cycle().

	   if the return port for insert has its own latency, we
	   need to take that into account too.
	*/

	return _session.engine().frames_per_cycle() + input_latency();
}

bool
PortInsert::can_support_input_configuration (ChanCount in) const
{
	if (input_maximum() == ChanCount::INFINITE && output_maximum() == ChanCount::INFINITE) {

		/* not configured yet */

		return true; /* we can support anything the first time we're asked */

	} else {

		/* the "input" config for a port insert corresponds to how
		   many output ports it will have.
		*/

		if (output_maximum() == in) {

			return true;
		} 
	}

	return false;
}

ChanCount
PortInsert::output_for_input_configuration (ChanCount in) const
{
	return in;
}

bool
PortInsert::configure_io (ChanCount in, ChanCount out)
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

	return (ensure_io (out, in, false, this) == 0);
}

ChanCount
PortInsert::output_streams() const
{
	return n_inputs ();
}

ChanCount
PortInsert::input_streams() const
{
	return n_outputs ();
}

