/*
 * This file is part of the Hybris programming language interpreter.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * Hybris is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hybris is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hybris.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _HVMEM_H_
#	define _HVMEM_H_

#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "object.h"

/* helper macro to obtain the address of a pointer */
#define H_ADDRESS_OF(o)      reinterpret_cast<unsigned long>(o)
/* default null value for an Object pointer */
#define H_UNDEFINED          NULL
/* anonymous identifier to be used upon temporary stacks creation */
#define HANONYMOUSIDENTIFIER     (char *)"HANONYMOUSIDENTIFIER"
#define HANONYMOUSIDENTIFIER_FTM (char *)"HANONYMOUSIDENTIFIER%d"

class VirtualMemory : public Map<Object> {
    public :

        VirtualMemory();
        ~VirtualMemory();

        Object *get( char *identifier );
        Object *add( char *identifier, Object *object );

        VirtualMemory *clone();

        void release();
};

/* post type definitions */
typedef VirtualMemory vmem_t;
typedef VirtualMemory vframe_t;

#endif
