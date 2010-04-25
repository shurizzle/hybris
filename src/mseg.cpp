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
#include "mseg.h"
#include "common.h"

MemorySegment::MemorySegment() : HashMap<Object>() {

}

MemorySegment::~MemorySegment(){
	/*
	 * See note on ~HashMap()
	 */

	/*
	 * Handle non managed exceptions.
	 */
	if( state._exception == true ){
		state._exception = false;
		assert( state.value != NULL );
		if( state.value->type->svalue ){
			hyb_error( H_ET_GREATER_EQNERIC, "Unhandled exception : %s", ob_svalue(state.value).c_str() );
		}
		else{
			hyb_error( H_ET_GREATER_EQNERIC, "Unhandled '%s' exception", ob_typename(state.value) );
		}
	}
}

Object *MemorySegment::add( char *identifier, Object *object ){
    Object *_new = H_UNDEFINED,
           *_old = H_UNDEFINED;

    if( object != H_UNDEFINED ){
    	/*
    	 * Only constants and referenced objects shall be cloned.
    	 */
    	if( (object->attributes & H_OA_CONSTANT) == H_OA_CONSTANT || object->ref > 0 ){
    		_new = ob_clone(object);
    	}
    	/*
    	 * Non constant and no references.
    	 */
    	else{
    		_new = object;
    	}

    	ob_set_references( _new, +1 );
    }

    /* if object does not exist yet, insert as a new one */
    if( (_old = get( identifier )) == H_UNDEFINED ){
        return insert( identifier, _new );
    }
    /* else set the new value */
    else{
		replace( identifier, _old, _new );

		ob_free(_old);

		return _new;
    }
}

MemorySegment *MemorySegment::clone(){
    unsigned int i;

    MemorySegment *clone = new MemorySegment;

    for( i = 0; i < m_elements; ++i ){
        clone->add( (char *)label(i), at(i) );
    }

	clone->state.assign(state);

    return clone;
}
