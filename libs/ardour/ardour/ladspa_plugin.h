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

#ifndef __ardour_ladspa_plugin_h__
#define __ardour_ladspa_plugin_h__

#include <set>
#include <vector>
#include <string>
#include <dlfcn.h>

#include <sigc++/signal.h>

#include <pbd/stateful.h> 

#include <jack/types.h>
#include <ardour/ladspa.h>
#include <ardour/plugin.h>

namespace ARDOUR {
class AudioEngine;
class Session;

class LadspaPlugin : public ARDOUR::Plugin
{
  public:
	LadspaPlugin (void *module, ARDOUR::AudioEngine&, ARDOUR::Session&, uint32_t index, nframes_t sample_rate);
	LadspaPlugin (const LadspaPlugin &);
	~LadspaPlugin ();

	/* Plugin interface */
	
	std::string unique_id() const;
	const char* label() const           { return _descriptor->Label; }
	const char* name() const            { return _descriptor->Name; }
	const char* maker() const           { return _descriptor->Maker; }
	uint32_t    parameter_count() const { return _descriptor->PortCount; }
	float       default_value (uint32_t port);
	nframes_t   signal_latency() const;
	void        set_parameter (uint32_t port, float val);
	float       get_parameter (uint32_t port) const;
	int         get_parameter_descriptor (uint32_t which, ParameterDescriptor&) const;
	uint32_t    nth_parameter (uint32_t port, bool& ok) const;
	
	std::set<Evoral::Parameter> automatable() const;

	void activate () { 
		if (!_was_activated && _descriptor->activate)
			_descriptor->activate (_handle);

		_was_activated = true;
	}

	void deactivate () {
		if (_was_activated && _descriptor->deactivate)
			_descriptor->deactivate (_handle);

		_was_activated = false;
	}

	void cleanup () {
		activate();
		deactivate();

		if (_descriptor->cleanup)
			_descriptor->cleanup (_handle);
	}

	void set_block_size (nframes_t nframes) {}
	
	int         connect_and_run (BufferSet& bufs, uint32_t& in, uint32_t& out, nframes_t nframes, nframes_t offset);
	std::string describe_parameter (Evoral::Parameter);
	std::string state_node_name() const { return "ladspa"; }
	void        print_parameter (uint32_t, char*, uint32_t len) const;

	bool parameter_is_audio(uint32_t) const;
	bool parameter_is_control(uint32_t) const;
	bool parameter_is_input(uint32_t) const;
	bool parameter_is_output(uint32_t) const;
	bool parameter_is_toggled(uint32_t) const;

	XMLNode& get_state();
	int      set_state(const XMLNode& node);
	bool     save_preset(std::string name);

	bool has_editor() const { return false; }

	int require_output_streams (uint32_t);
	
	/* LADSPA extras */

	LADSPA_Properties           properties() const                { return _descriptor->Properties; }
	uint32_t                    index() const                     { return _index; }
	const char *                copyright() const                 { return _descriptor->Copyright; }
	LADSPA_PortDescriptor       port_descriptor(uint32_t i) const { return _descriptor->PortDescriptors[i]; }
	const LADSPA_PortRangeHint* port_range_hints() const          { return _descriptor->PortRangeHints; }
	const char * const *        port_names() const                { return _descriptor->PortNames; }
	
	void set_gain (float gain)                    { _descriptor->set_run_adding_gain (_handle, gain); }
	void run_adding (uint32_t nsamples)           { _descriptor->run_adding (_handle, nsamples); }
	void connect_port (uint32_t port, float *ptr) { _descriptor->connect_port (_handle, port, ptr); }

  private:
	void*                    _module;
	const LADSPA_Descriptor* _descriptor;
	LADSPA_Handle            _handle;
	nframes_t                _sample_rate;
	LADSPA_Data*             _control_data;
	LADSPA_Data*             _shadow_data;
	LADSPA_Data*             _latency_control_port;
	uint32_t                 _index;
	bool                     _was_activated;

	void init (void *mod, uint32_t index, nframes_t rate);
	void run_in_place (nframes_t nsamples);
	void latency_compute_run ();
};

class LadspaPluginInfo : public PluginInfo {
  public:	
	LadspaPluginInfo () { };
	~LadspaPluginInfo () { };

	PluginPtr load (Session& session);
};

typedef boost::shared_ptr<LadspaPluginInfo> LadspaPluginInfoPtr;

} // namespace ARDOUR

#endif /* __ardour_ladspa_plugin_h__ */
