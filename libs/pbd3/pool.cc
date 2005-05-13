/*
    Copyright (C) 1998-99 Paul Barton-Davis
 
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

#include <iostream>
#include <sys/mman.h>
#include <vector>

#include <pbd/pool.h>
#include <pbd/error.h>
#include <pbd/stl_delete.h>
#include <pbd/pthread_utils.h>

using namespace std;

Pool::Pool (string n, unsigned long item_size, unsigned long nitems)
{
	_name = n;

	free_list = new RingBuffer<void*> (nitems);

	/* since some overloaded ::operator new() might use this,
	   its important that we use a "lower level" allocator to
	   get more space.  
	*/

	block = malloc (nitems * item_size);

	void **ptrlist = (void **) malloc (sizeof (void *)  * nitems);

	for (unsigned long i = 0; i < nitems; i++) {
		ptrlist[i] = static_cast<void *> (static_cast<char*>(block) + (i * item_size));
	}

	free_list->write (ptrlist, nitems);

	free (ptrlist);
}

Pool::~Pool ()
{
	free (block);
}

void *
Pool::alloc ()
{
	void *ptr;

//	cerr << _name << " pool " << " alloc, thread = " << pthread_name() << " space = " << free_list->read_space() << endl;

	if (free_list->read (&ptr, 1) < 1) {
		fatal << "CRITICAL: " << _name << " POOL OUT OF MEMORY - RECOMPILE WITH LARGER SIZE!!" << endmsg;
		/*NOTREACHED*/
		return 0;
	} else {
		return ptr;
	}
};

void		
Pool::release (void *ptr)
{
	free_list->write (&ptr, 1);
//	cerr << _name << ": release, now has " << free_list->read_space() << endl;
}

/*---------------------------------------------*/

MultiAllocSingleReleasePool::MultiAllocSingleReleasePool (string n, unsigned long isize, unsigned long nitems) 
	: Pool (n, isize, nitems)
{
	pthread_mutex_init (&lock, 0);
}

MultiAllocSingleReleasePool::~MultiAllocSingleReleasePool ()
{
}

SingleAllocMultiReleasePool::SingleAllocMultiReleasePool (string n, unsigned long isize, unsigned long nitems) 
	: Pool (n, isize, nitems)
{
	pthread_mutex_init (&lock, 0);
}

SingleAllocMultiReleasePool::~SingleAllocMultiReleasePool ()
{
}

void*
MultiAllocSingleReleasePool::alloc ()
{
	void *ptr;
	pthread_mutex_lock (&lock);
	ptr = Pool::alloc ();
	pthread_mutex_unlock (&lock);
	return ptr;
}

void
MultiAllocSingleReleasePool::release (void* ptr)
{
	Pool::release (ptr);
}

void*
SingleAllocMultiReleasePool::alloc ()
{
	return Pool::alloc ();
}

void
SingleAllocMultiReleasePool::release (void* ptr)
{
	pthread_mutex_lock (&lock);
	Pool::release (ptr);
	pthread_mutex_unlock (&lock);
}

