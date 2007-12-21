#include <map>

#include <gtkmm/stock.h>
#include <gtkmm/accelkey.h>
#include <gtkmm/accelmap.h>
#include <gtkmm/uimanager.h>

#include <pbd/strsplit.h>

#include "actions.h"
#include "keyboard.h"
#include "keyeditor.h"

#include "i18n.h"

using namespace std;
using namespace Gtk;
using namespace Gdk;

KeyEditor::KeyEditor ()
	: ArdourDialog (_("Keybinding Editor"), false)
{
	can_bind = false;
	last_state = 0;

	model = TreeStore::create(columns);

	view.set_model (model);
	view.append_column (_("Action"), columns.action);
	view.append_column (_("Binding"), columns.binding);
	view.set_headers_visible (true);
	view.get_selection()->set_mode (SELECTION_SINGLE);
	view.set_reorderable (false);
	view.set_size_request (300,200);
	view.set_enable_search (false);
	view.set_rules_hint (true);
	view.set_name (X_("KeyEditorTree"));

	view.get_selection()->signal_changed().connect (mem_fun (*this, &KeyEditor::action_selected));
	
	scroller.add (view);
	scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	get_vbox()->pack_start (scroller);
	get_vbox()->set_border_width (12);

	scroller.show ();
	view.show ();
}

void
KeyEditor::on_show ()
{
	populate ();
	view.get_selection()->unselect_all ();
	ArdourDialog::on_show ();
}

void
KeyEditor::on_unmap ()
{
	ArdourDialog::on_unmap ();
}

void
KeyEditor::action_selected ()
{
}

bool
KeyEditor::on_key_press_event (GdkEventKey* ev)
{
	can_bind = true;
	last_state = ev->state;
	return false;
}

bool
KeyEditor::on_key_release_event (GdkEventKey* ev)
{
	if (!can_bind || ev->state != last_state) {
		return false;
	}

	TreeModel::iterator i = view.get_selection()->get_selected();

	if (i != model->children().end()) {
		string path = (*i)[columns.path];
		
		if (!(*i)[columns.bindable]) {
			goto out;
		} 

		bool result = AccelMap::change_entry (path,
						      ev->keyval,
						      (ModifierType) ev->state,
						      true);

		if (result) {
			bool known;
			AccelKey key;

			known = ActionManager::lookup_entry (path, key);
			
			if (known) {
				(*i)[columns.binding] = ActionManager::ui_manager->get_accel_group()->name (key.get_key(), Gdk::ModifierType (key.get_mod()));
			} else {
				(*i)[columns.binding] = string();
			}
		}

		
	}

  out:
	can_bind = false;
	return true;
}

void
KeyEditor::populate ()
{
	vector<string> paths;
	vector<string> labels;
	vector<string> keys;
	vector<AccelKey> bindings;
	typedef std::map<string,TreeIter> NodeMap;
	NodeMap nodes;
	NodeMap::iterator r;
	
	ActionManager::get_all_actions (labels, paths, keys, bindings);
	
	vector<string>::iterator k;
	vector<string>::iterator p;
	vector<string>::iterator l;

	model->clear ();

	for (l = labels.begin(), k = keys.begin(), p = paths.begin(); l != labels.end(); ++k, ++p, ++l) {
		
		TreeModel::Row row;
		vector<string> parts;
		
		parts.clear ();

		split (*p, parts, '/');
		
		if (parts.empty()) {
			continue;
		}

		if ((r = nodes.find (parts[1])) == nodes.end()) {

			/* top level is missing */

			TreeIter rowp;
			TreeModel::Row parent;
			rowp = model->append();
			nodes[parts[1]] = rowp;
			parent = *(rowp);
			parent[columns.action] = parts[1];
			parent[columns.bindable] = false;

			row = *(model->append (parent.children()));

		} else {
			
			row = *(model->append ((*r->second)->children()));

		}
		
		/* add this action */

		row[columns.action] = (*l);
		row[columns.path] = (*p);
		row[columns.bindable] = true;
		
		if (*k == ActionManager::unbound_string) {
			row[columns.binding] = string();
		} else {
			row[columns.binding] = (*k);
		}
	}
}