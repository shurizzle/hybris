/*
 * This file is part of hybris.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * hybris is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hybris is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hybris.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "common.h"
#include "vmem.h"
#include "builtin.h"
#include "tree.h"

extern Object *htree_function_call( vmem_t *stackframe, Node *call, int threaded = 0 );

unsigned long h_running_threads = 0;

void * hybris_pthread_worker( void *arg ){
    __sync_fetch_and_add( &h_running_threads, 1 );

    vmem_t *data = (vmem_t *)arg;

    Node *call  = new Node(H_NT_CALL);
    call->_call = data->at(0)->xstring;
	if( data->size() > 1 ){
		unsigned int i;
		for( i = 1; i < data->size(); i++ ){
			switch( data->at(i)->xtype ){
				case H_OT_INT    : call->addChild( Tree::addInt(data->at(i)->xint) ); break;
				case H_OT_FLOAT  : call->addChild( Tree::addFloat(data->at(i)->xfloat) ); break;
				case H_OT_CHAR   : call->addChild( Tree::addChar(data->at(i)->xchar) ); break;
				case H_OT_STRING : call->addChild( Tree::addString((char *)data->at(i)->xstring.c_str()) ); break;
				default :
                    __sync_fetch_and_sub( &h_running_threads, 1 );
                    hybris_generic_error( "type not supported for pthread call" );
			}
		}
	}

	Object *_return = htree_function_call( data, call, 1 );
	delete call;
    delete _return;

    #ifdef GC_SUPPORT
        hybris_vm_release( data, NULL );
    #else
        hybris_vm_release( data );
    #endif

    __sync_fetch_and_sub( &h_running_threads, 1 );

    pthread_exit(NULL);
}

HYBRIS_BUILTIN(hpthread_create){
	if( data->size() < 1 ){
		hybris_syntax_error( "function 'pthread_create' requires at least 1 parameter (called with %d)", data->size() );
	}
	htype_assert( data->at(0), H_OT_STRING );

    pthread_t tid;
    pthread_create( &tid, NULL, hybris_pthread_worker, (void *)hybris_vm_clone(data) );

    return new Object( static_cast<long>(tid) );
}

HYBRIS_BUILTIN(hpthread_exit){
	pthread_exit(NULL);
    return NULL;
}

HYBRIS_BUILTIN(hpthread_join){
    if( data->size() < 1 ){
		hybris_syntax_error( "function 'pthread_join' requires at least 1 parameter (called with %d)", data->size() );
	}
	htype_assert( data->at(0), H_OT_INT );

    pthread_t tid = static_cast<pthread_t>( data->at(0)->xint );
    void *status;

    pthread_join( tid, &status );
    return NULL;
}

