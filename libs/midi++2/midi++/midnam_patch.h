/*
    Copyright (C) 2008 Hans Baier 

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

#ifndef MIDNAM_PATCH_H_
#define MIDNAM_PATCH_H_

#include <string>
#include <list>
#include <set>
#include <map>

#include "pbd/stateful.h"
#include "midi++/event.h"
#include "pbd/xml++.h"

namespace MIDI
{

namespace Name
{

struct PatchPrimaryKey
{
public:
	int msb;
	int lsb;
	int program_number;
	
	PatchPrimaryKey(int a_msb = -1, int a_lsb = -1, int a_program_number = -1) {
		msb = a_msb;
		lsb = a_lsb;
		program_number = a_program_number;
	}
	
	bool is_sane() { 	
		return ((msb >= 0) && (msb <= 127) &&
			(lsb >= 0) && (lsb <= 127) &&
			(program_number >=0 ) && (program_number <= 127));
	}
	
	inline PatchPrimaryKey& operator=(const PatchPrimaryKey& id) {
		msb = id.msb;
		lsb = id.lsb; 
		program_number = id.program_number;
		return *this;
	}
	
	inline bool operator==(const PatchPrimaryKey& id) const {
		return (msb == id.msb && lsb == id.lsb && program_number == id.program_number);
	}
	
	/**
	 * obey strict weak ordering or crash in STL containers
	 */
	inline bool operator<(const PatchPrimaryKey& id) const {
		if (msb < id.msb) {
			return true;
		} else if (msb == id.msb && lsb < id.lsb) {
			return true;
		} else if (lsb == id.lsb && program_number < id.program_number) {
			return true;
		}
		
		return false;
	}
};

class Patch : public PBD::Stateful
{
public:
	typedef std::list<boost::shared_ptr<Evoral::MIDIEvent> > PatchMidiCommands;

	Patch() {};
	Patch(string a_number, string a_name) : _number(a_number), _name(a_name) {};
	~Patch() {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	const string& number() const             { return _number; }
	void set_number(const string a_number)   { _number = a_number; }

	const PatchMidiCommands& patch_midi_commands() const { return _patch_midi_commands; }
	
	const PatchPrimaryKey&   patch_primary_key()   const { return _id; }

	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string            _number;
	string            _name;
	PatchPrimaryKey   _id;
	PatchMidiCommands _patch_midi_commands;
};

class PatchBank : public PBD::Stateful
{
public:
	typedef std::list<boost::shared_ptr<Patch> > PatchNameList;

	PatchBank() {};
	virtual ~PatchBank() {};
	PatchBank(string a_name) : _name(a_name) {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	const PatchNameList& patch_name_list() const { return _patch_name_list; }

	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string        _name;
	PatchNameList _patch_name_list;
};

#include <iostream>

class ChannelNameSet : public PBD::Stateful
{
public:
	typedef std::set<uint8_t>                                    AvailableForChannels;
	typedef std::list<boost::shared_ptr<PatchBank> >             PatchBanks;
	typedef std::map<PatchPrimaryKey, boost::shared_ptr<Patch> > PatchMap;
	typedef std::list<PatchPrimaryKey>                           PatchList;

	ChannelNameSet() {};
	virtual ~ChannelNameSet() {};
	ChannelNameSet(string a_name) : _name(a_name) {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	bool available_for_channel(uint8_t channel) const { 
		return _available_for_channels.find(channel) != _available_for_channels.end(); 
	}
	
	boost::shared_ptr<Patch> find_patch(PatchPrimaryKey& key) {
		assert(key.is_sane());
		return _patch_map[key];
	}
	
	boost::shared_ptr<Patch> previous_patch(PatchPrimaryKey& key) {
		assert(key.is_sane());
		std::cerr << "finding patch with "  << key.msb << "/" << key.lsb << "/" <<key.program_number << std::endl; 
		for (PatchList::const_iterator i = _patch_list.begin();
			 i != _patch_list.end();
			 ++i) {
			if ((*i) == key) {
				if (i != _patch_list.begin()) {
					std::cerr << "got it!" << std::endl;
					--i;
					return  _patch_map[*i];
				} 
			}
		}
			
		return boost::shared_ptr<Patch>();
	}
	
	boost::shared_ptr<Patch> next_patch(PatchPrimaryKey& key) {
		assert(key.is_sane());
		std::cerr << "finding patch with "  << key.msb << "/" << key.lsb << "/" <<key.program_number << std::endl; 
		for (PatchList::const_iterator i = _patch_list.begin();
			 i != _patch_list.end();
			 ++i) {
			if ((*i) == key) {
				if (++i != _patch_list.end()) {
					std::cerr << "got it!" << std::endl;
					return  _patch_map[*i];
				} else {
					--i;
				}
			}
		}
			
		return boost::shared_ptr<Patch>();
	}

	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string _name;
	AvailableForChannels _available_for_channels;
	PatchBanks           _patch_banks;
	PatchMap             _patch_map;
	PatchList            _patch_list;
};

class Note : public PBD::Stateful
{
public:
	Note() {};
	Note(string a_number, string a_name) : _number(a_number), _name(a_name) {};
	~Note() {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	const string& number() const             { return _number; }
	void set_number(const string a_number)   { _number = a_number; }

	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string _number;
	string _name;
};

class NoteNameList : public PBD::Stateful
{
public:
	typedef std::list<boost::shared_ptr<Note> > Notes;
	NoteNameList() {};
	NoteNameList(string a_name) : _name(a_name) {};
	~NoteNameList() {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	const Notes& notes() const { return _notes; }

	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string _name;
	Notes  _notes;
};

class CustomDeviceMode : public PBD::Stateful
{
public:
	CustomDeviceMode() {};
	virtual ~CustomDeviceMode() {};

	const string& name() const               { return _name; }
	void set_name(const string a_name)       { _name = a_name; }

	
	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);
	
	string channel_name_set_name_by_channel(uint8_t channel) {
		assert(channel <= 15);
		return _channel_name_set_assignments[channel]; 
	}
	
private:
	/// array index = channel number
	/// string contents = name of channel name set 
	string _name;
	string _channel_name_set_assignments[16];
};

class MasterDeviceNames : public PBD::Stateful
{
public:
	typedef std::list<std::string>                                       Models;
	/// maps name to CustomDeviceMode
	typedef std::map<std::string, boost::shared_ptr<CustomDeviceMode> >  CustomDeviceModes;
	typedef std::list<std::string>                                       CustomDeviceModeNames;
	/// maps name to ChannelNameSet
	typedef std::map<std::string, boost::shared_ptr<ChannelNameSet> >    ChannelNameSets;
	typedef std::list<boost::shared_ptr<NoteNameList> >                  NoteNameLists;
	
	
	MasterDeviceNames() {};
	virtual ~MasterDeviceNames() {};
	
	const string& manufacturer() const { return _manufacturer; }
	void set_manufacturer(const string a_manufacturer) { _manufacturer = a_manufacturer; }
	
	const Models& models() const { return _models; }
	void set_models(const Models some_models) { _models = some_models; }
	
	const CustomDeviceModeNames& custom_device_mode_names() const { return _custom_device_mode_names; }
	
	boost::shared_ptr<CustomDeviceMode> custom_device_mode_by_name(string mode_name) {
		assert(mode_name != "");
		return _custom_device_modes[mode_name];
	}
	
	boost::shared_ptr<ChannelNameSet> channel_name_set_by_device_mode_and_channel(string mode, uint8_t channel) {
		return _channel_name_sets[custom_device_mode_by_name(mode)->channel_name_set_name_by_channel(channel)];
	}
	
	boost::shared_ptr<Patch> find_patch(string mode, uint8_t channel, PatchPrimaryKey& key) {
		return channel_name_set_by_device_mode_and_channel(mode, channel)->find_patch(key);
	}
	
	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);
	
private:
	string                _manufacturer;
	Models                _models;
	CustomDeviceModes     _custom_device_modes;
	CustomDeviceModeNames _custom_device_mode_names;
	ChannelNameSets       _channel_name_sets;
	NoteNameLists         _note_name_lists;
};

class MIDINameDocument : public PBD::Stateful
{
public:
	// Maps Model names to MasterDeviceNames
	typedef std::map<std::string, boost::shared_ptr<MasterDeviceNames> > MasterDeviceNamesList;
	
	MIDINameDocument() {};
	MIDINameDocument(const string &filename) : _document(XMLTree(filename)) { set_state(*_document.root()); };
	virtual ~MIDINameDocument() {};

	const string& author() const { return _author; }
	void set_author(const string an_author) { _author = an_author; }
	
	const MasterDeviceNamesList& master_device_names_by_model() const { return _master_device_names_list; }
	
	const MasterDeviceNames::Models& all_models() const { return _all_models; }
		
	XMLNode& get_state (void);
	int      set_state (const XMLNode& a_node);

private:
	string                        _author;
	MasterDeviceNamesList         _master_device_names_list;
	XMLTree                       _document;
	MasterDeviceNames::Models     _all_models;
};

}

}
#endif /*MIDNAM_PATCH_H_*/
