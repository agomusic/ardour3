#include <pbd/error.h>
#include <ardour/state_manager.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace std;

bool StateManager::_allow_save = true;
set<StateManager*> StateManager::deferred;

StateManager::StateManager ()
{
	_current_state_id = 0;
}

StateManager::~StateManager()
{
}

void
StateManager::prohibit_save ()
{
	_allow_save = false;
}

void
StateManager::allow_save (const char* why, bool do_save)
{
	_allow_save = true;
	if (do_save) {
		for (set<StateManager*>::iterator x = deferred.begin(); x != deferred.end(); ++x) {
			(*x)->save_state (why);
		}
	}
	deferred.clear ();
}

void
StateManager::drop_all_states ()
{
	for (StateMap::iterator i = states.begin(); i != states.end(); ++i) {
		delete *i;
	}

	states.clear ();

	save_state (_("cleared history"));
}

void
StateManager::use_state (state_id_t id)
{
	Change what_changed;
	state_id_t n;
	StateMap::iterator i;

	for (n = 0, i = states.begin(); n < id && i != states.end(); ++n, ++i);

	if (n != id || i == states.end()) {
		fatal << string_compose (_("programming error: illegal state ID (%1) passed to "
				    "StateManager::set_state() (range = 0-%2)"), id, states.size()-1)
		      << endmsg;
		/*NOTREACHED*/
		return;
	}

	what_changed = restore_state (**i);
	_current_state_id = id;
	send_state_changed (what_changed);
}

void
StateManager::save_state (std::string why)
{
	if (!_allow_save) {
		deferred.insert (this);
		return;
	}

	states.push_back (state_factory (why));
	_current_state_id = states.size() - 1;
}

void
StateManager::send_state_changed (Change what_changed)
{
	StateChanged (what_changed);
}
