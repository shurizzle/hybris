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
#ifndef _HGC_H_
#	define _HGC_H_

#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

/*
 * Force specified functions to be inlined by the compiler.
 */
#ifndef __force_inline
#	define __force_inline __inline__ __attribute__((always_inline))
#endif

typedef unsigned long ulong;

/*
 * This is a mark-and-sweep garbage collector implementation.
 *
 * When the global memory usage is >= the threshold defined
 * down below, or with command line parameter, the gc will be
 * triggered and will mark all alive objects (the ones in alive
 * memory frames) and their children if any (for collections),
 * then it will loop every object in the heap and free unmarked
 * objects.
 *
 * NOTE:
 * To make this work, ALL the new objects should be passed
 * instantly to the gc_track function, so use the gc_new_* macros instead
 * of new operator or malloc C function to create an object.
 * No manually deletion is necessary.
 */
#define GC_DEFAULT_MEMORY_THRESHOLD 2048000
/*
 * Maximum allowed memory size usage, if this threshold is reached
 * by __gc.usage counter, a fatal error will be triggered.
 *
 * Default value: 128M
 */
#define GC_ALLOWED_MEMORY_THRESHOLD	134217728

struct _Object;
struct _vm_t;

/*
 * This structure represent an item in the gc 
 * pool.
 *
 * pobj 	: Is a pointer to the object to track.
 * size     : The size of the object itself.
 * gc_count : Incremented when an object is not freed after a
 * 		      gc_collect cycle (for future multi generation gc).
 * next 	: Pointer to the next item in the pool.
 * prev 	: Pointer to the previous item in the pool.
 */
typedef struct _gc_item {
    struct _Object *pobj;
    size_t          size;
    size_t			gc_count;

    _gc_item       *next;
    _gc_item       *prev;

    _gc_item( struct _Object *p, size_t s ) : pobj(p), size(s), gc_count(0) { }
}
gc_item_t;

typedef struct _gc_list {
	gc_item_t *head;
	gc_item_t *tail;

	size_t	   items;
	size_t	   usage;

	_gc_list() : head(NULL), tail(NULL), items(0), usage(0){ }
}
gc_list_t;

/*
 * Threshold upon which an object is moved from the heap
 * space to the lag space, where 0.7 is 70% object collections
 * on the total collections counter.
 */
#define GC_LAGGING_THRESHOLD 		  0.7f
/*
 * Determine if an object has to be moved to the lag space.
 */
#define GC_IS_LAGGING(v)     		  v / (double)__gc.collections >= GC_LAGGING_THRESHOLD

/*
 * Main gc structure, kind of the "head" of the pool.
 *
 * constants    : Constant objects list (will be freed at the end).
 * lag			: When an object remains alive in the heap for a given
 * 				  amount of collections, it's going to be moved to this
 * 				  lag space.
 * heap         : Heap objects list.
 * collections  : Collection cycles counter.
 * items	    : Number of items in the pool.
 * usage	    : Global memory usage, in bytes.
 * gc_threshold : If usage >= this, the gc is triggered.
 * mm_threshold : If usage >= this, a memory exhausted error is triggered.
 * mutex        : Mutex to lock the pool while collecting.
 */
typedef struct _gc {
	gc_list_t		constants;
	gc_list_t		lag;
	gc_list_t		heap;
	size_t			collections;
    size_t     		items;
    size_t     		usage;
    size_t     	 	gc_threshold;
    size_t			mm_threshold;
	pthread_mutex_t mutex;

	_gc(){
		collections  = 0;
		items 		 = 0;
		usage        = 0;
		gc_threshold = GC_DEFAULT_MEMORY_THRESHOLD;
		mm_threshold = GC_ALLOWED_MEMORY_THRESHOLD;
		mutex        = PTHREAD_MUTEX_INITIALIZER;
	}
}
gc_t;

/*
 * Set the 'gc_threshold' attribute of the gc structure.
 * Return the old threshold value.
 */
size_t			gc_set_collect_threshold( size_t threshold );
/*
 * Set the 'mm_threshold' attribute of the gc structure.
 * Return the old threshold value.
 */
size_t			gc_set_mm_threshold( size_t threshold );
/* 
 * Add an object to the gc pool and start to track
 * it for reference changes.
 * Size must be passed explicitly due to the downcasting
 * possibility.
 */
struct _Object *gc_track( struct _Object *o, size_t size );
/*
 * Return the number of objects tracked by the gc.
 */
size_t			gc_mm_items();
/*
 * Return the actual memory usage in bytes.
 */
size_t          gc_mm_usage();
/*
 * Return the threshold value upon which the gc the collect routine
 * will be triggered.
 */
size_t			gc_collect_threshold();
/*
 * Return the maximum allowed memory usage threshold.
 */
size_t			gc_mm_threshold();
/*
 * Recursively mark an object (and its inner items).
 */
void 			gc_mark( struct _Object *o, bool mark = true );
/*
 * Set the object and all the objects referenced by him
 * as non collectable.
 */
#define	gc_set_alive(o) gc_mark( o, true )
/*
 * Set the object and all the objects referenced by him
 * as collectable.
 */
#define gc_set_dead(o)  gc_mark( o, false )
/*
 * Fire the collection routines if the memory usage is
 * above the threshold.
 */
void            gc_collect( struct _vm_t *vm );
/*
 * Release all the pool and its contents, should be
 * used when the program is exiting, not before.
 */
void            gc_release();

/*
 * Object allocation macros.
 *
 * 1 .: Alloc new specialized type pointer.
 * 2 .: Downcast to Object * and let the gc track it.
 * 3 .: Upcast back to specialized type pointer and return to user.
 */
#define gc_new_boolean(v)    (BooleanObject *)   gc_track( (Object *)( new BooleanObject( static_cast<bool>(v) ) ), 	 sizeof(BooleanObject) )
#define gc_new_integer(v)    (IntegerObject *)   gc_track( (Object *)( new IntegerObject( static_cast<long>(v) ) ), 	 sizeof(IntegerObject) )
#define gc_new_alias(v)      (AliasObject *)     gc_track( (Object *)( new AliasObject( static_cast<long>(v) ) ),   	 sizeof(AliasObject) )
#define gc_new_extern(v)     (ExternObject *)    gc_track( (Object *)( new ExternObject( static_cast<long>(v) ) ),  	 sizeof(ExternObject) )
#define gc_new_float(v)      (FloatObject *)     gc_track( (Object *)( new FloatObject( static_cast<double>(v) ) ), 	 sizeof(FloatObject) )
#define gc_new_char(v)       (CharObject *)      gc_track( (Object *)( new CharObject( static_cast<char>(v) ) ),    	 sizeof(CharObject) )
#define gc_new_string(v)     (StringObject *)    gc_track( (Object *)( new StringObject( (char *)(v) ) ),           	 sizeof(StringObject) )
#define gc_new_binary(d)     (BinaryObject *)    gc_track( (Object *)( new BinaryObject(d) ),                       	 sizeof(BinaryObject) )
#define gc_new_vector()      (VectorObject *)    gc_track( (Object *)( new VectorObject() ),                        	 sizeof(VectorObject) )
#define gc_new_map()         (MapObject *)       gc_track( (Object *)( new MapObject() ),                           	 sizeof(MapObject) )
#define gc_new_struct()      (StructureObject *) gc_track( (Object *)( new StructureObject() ),                     	 sizeof(StructureObject) )
#define gc_new_class()       (ClassObject *)     gc_track( (Object *)( new ClassObject() ),                              sizeof(ClassObject) )
#define gc_new_reference(o)  (ReferenceObject *) gc_track( (Object *)( new ReferenceObject(o) ),                         sizeof(ReferenceObject) )
#define gc_new_handle(o)     (HandleObject *)    gc_track( (Object *)( new HandleObject( reinterpret_cast<void *>(o)) ), sizeof(HandleObject) )

#endif
