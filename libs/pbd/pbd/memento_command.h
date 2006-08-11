/* 
   Copyright (C) 2006 Hans Fugal & Paul Davis

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

    $Id: /local/undo/libs/pbd3/pbd/undo.h 132 2006-06-29T18:45:16.609763Z fugalh  $
*/

#ifndef __lib_pbd_memento_command_h__
#define __lib_pbd_memento_command_h__

#include <pbd/command.h>
#include <pbd/xml++.h>
#include <sigc++/slot.h>
#include <typeinfo>

/** This command class is initialized with before and after mementos 
 * (from Stateful::get_state()), so undo becomes restoring the before
 * memento, and redo is restoring the after memento.
 */
template <class obj_T>
class MementoCommand : public Command
{
    public:
	MementoCommand(XMLNode &state);
        MementoCommand(obj_T &obj, 
                       XMLNode &before,
                       XMLNode &after
                       ) 
            : obj(obj), before(before), after(after) {}
        void operator() () { obj.set_state(after); }
        void undo() { obj.set_state(before); }
        virtual XMLNode &get_state() 
        {
            XMLNode *node = new XMLNode("MementoCommand");
            node->add_property("obj_id", obj.id().to_s());
            node->add_property("type_name", typeid(obj).name());
            node->add_child_copy(before);
            node->add_child_copy(after);
            return *node;
        }
    protected:
        obj_T &obj;
        XMLNode &before, &after;
};

template <class obj_T>
class MementoUndoCommand : public Command
{
public:
    MementoUndoCommand(XMLNode &state);
    MementoUndoCommand(obj_T &obj, 
                       XMLNode &before)
        : obj(obj), before(before) {}
    void operator() () { /* noop */ }
    void undo() { obj.set_state(before); }
    virtual XMLNode &get_state() 
    {
        XMLNode *node = new XMLNode("MementoUndoCommand");
        node->add_property("obj_id", obj.id().to_s());
	node->add_property("type_name", typeid(obj).name());
        node->add_child_copy(before);
        return *node;
    }
protected:
    obj_T &obj;
    XMLNode &before;
};

template <class obj_T>
class MementoRedoCommand : public Command
{
public:
    MementoRedoCommand(XMLNode &state);
    MementoRedoCommand(obj_T &obj, 
                       XMLNode &after)
        : obj(obj), after(after) {}
    void operator() () { obj.set_state(after); }
    void undo() { /* noop */ }
    virtual XMLNode &get_state()
    {
        XMLNode *node = new XMLNode("MementoRedoCommand");
        node->add_property("obj_id", obj.id().to_s());
	node->add_property("type_name", typeid(obj).name());
        node->add_child_copy(after);
        return *node;
    }
protected:
    obj_T &obj;
    XMLNode &after;
};

#endif // __lib_pbd_memento_h__
