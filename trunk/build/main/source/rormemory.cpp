/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 1st of February 2010

#include "rormemory.h"

unsigned int bytes_allocated = 0;

void *ror_malloc(size_t size)
{
	void *addr = nedalloc::nedmalloc(size);
	bytes_allocated += size;
	return addr;
}

void *ror_calloc(size_t no, size_t size)
{
	void *addr = nedalloc::nedcalloc(no, size);
	bytes_allocated += size;
	return addr;
}

void ror_free(void *mem)
{
	// wont work like this ...
	//bytes_allocated -= size
	//allocations.erase(mem);
	nedalloc::nedfree(mem);
}

unsigned long getMemoryAllocated()
{
	return bytes_allocated;
}