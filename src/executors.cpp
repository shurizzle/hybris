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
#include "executors.h"

Object *exec_identifier( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED;
    int     idx;
    char   *identifier = (char *)node->_identifier.c_str();

    // search for the identifier on the function frame
    o = hybris_vm_get( frame, identifier );
    if( o == H_UNDEFINED && H_ADDRESS_OF(frame) != H_ADDRESS_OF(&ctx->vmem) ){
        // search it on the global frame if it's different from local frame
        o = hybris_vm_get( &ctx->vmem, identifier );
    }
    // search for it as a function name
    if( o == H_UNDEFINED ){
        idx = ctx->vcode.index( identifier );
        if( idx != -1 ){
            o = new Object((unsigned int)idx);
        }
        // identifier not found
        else{
            hybris_syntax_error( "'%s' undeclared identifier", identifier );
        }
    }

    return o;
}

Object *exec_function( h_context_t *ctx, vmem_t *frame, Node *node ){
    /* check for double definition */
    if( hybris_vc_get( &ctx->vcode, (char *)node->_function.c_str() ) != H_UNDEFINED ){
        hybris_syntax_error( "function '%s' already defined", node->_function.c_str() );
    }
    else if( hfunction_search( ctx, (char *)node->_function.c_str() ) != H_UNDEFINED ){
        hybris_syntax_error( "function '%s' already defined as a language builtin", node->_function.c_str() );
    }
    /* add the function to the code segment */
    hybris_vc_add( &ctx->vcode, node );

    return H_UNDEFINED;
}

Object *exec_dollar( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o    = H_UNDEFINED,
           *name = H_UNDEFINED;

    o    = htree_execute( ctx,  frame, node->child(0) );
    name = o->toString();

    H_FREE_GARBAGE(o);

    if( (o = hybris_vm_get( frame, (char *)name->xstring.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", (char *)name->xstring.c_str() );
    }

    H_FREE_GARBAGE(name);

    return o;
}

Object *exec_pointer( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o   = H_UNDEFINED,
           *res = H_UNDEFINED;

    o   = htree_execute( ctx, frame, node->child(0) );
    res = new Object( (unsigned int)( H_ADDRESS_OF(o) ) );

    H_FREE_GARBAGE(o);

    return res;
}

Object *exec_object( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o   = H_UNDEFINED,
           *res = H_UNDEFINED;

    o   = htree_execute( ctx, frame, node->child(0) );
    res = o->getObject();

    H_FREE_GARBAGE(o);

    return res;
}

Object *exec_return( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED;

    o = htree_execute( ctx,  frame, node->child(0) );
    return o;
}

Object *exec_range( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *range = H_UNDEFINED,
           *from  = H_UNDEFINED,
           *to    = H_UNDEFINED;

    from  = htree_execute( ctx,  frame, node->child(0) );
    to    = htree_execute( ctx,  frame, node->child(1) );
    range = from->range( to );

    H_FREE_GARBAGE(from);
    H_FREE_GARBAGE(to);

    return range;
}

Object *exec_subscript_add( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *array  = H_UNDEFINED,
           *object = H_UNDEFINED,
           *res    = H_UNDEFINED;

    array  = htree_execute( ctx, frame, node->child(0) );
    object = htree_execute( ctx, frame, node->child(1) );
    res    = array->push(object);

    H_FREE_GARBAGE(array);
    H_FREE_GARBAGE(object);

    return res;
}

Object *exec_subscript_get( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *identifier = H_UNDEFINED,
           *array      = H_UNDEFINED,
           *index      = H_UNDEFINED,
           *result     = H_UNDEFINED;

    if( node->children() == 3 ){
        identifier    = htree_execute( ctx, frame, node->child(0) );
        array         = htree_execute( ctx, frame, node->child(1) );
        index         = htree_execute( ctx, frame, node->child(2) );
        (*identifier) = array->at( index );
        result        = identifier;

        H_FREE_GARBAGE(array);
    }
    else{
        array  = htree_execute( ctx, frame, node->child(0) );
        index  = htree_execute( ctx, frame, node->child(1) );
        result = array->at( index );
    }

    H_FREE_GARBAGE(index);

    return result;
}

Object *exec_subscript_set( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *array  = H_UNDEFINED,
           *index  = H_UNDEFINED,
           *object = H_UNDEFINED;

    array  = htree_execute( ctx, frame, node->child(0) );
    index  = htree_execute( ctx, frame, node->child(1) );
    object = htree_execute( ctx, frame, node->child(2) );

    array->at( index, object );

    H_FREE_GARBAGE(object);
    H_FREE_GARBAGE(index);

    return array;
}

Object *exec_while( h_context_t *ctx, vmem_t *frame, Node *node ){
    Node *condition,
         *body;

    Object *boolean = H_UNDEFINED,
           *result  = H_UNDEFINED;

    condition = node->child(0);
    body      = node->child(1);

    while( (boolean = htree_execute( ctx,  frame, condition ))->lvalue() ){
        result = htree_execute( ctx, frame, body );
        H_FREE_GARBAGE(result);
        H_FREE_GARBAGE(boolean);
    }
    H_FREE_GARBAGE(boolean);

    return H_UNDEFINED;
}

Object *exec_do( h_context_t *ctx, vmem_t *frame, Node *node ){
    Node *condition,
         *body;

    Object *boolean = H_UNDEFINED,
           *result  = H_UNDEFINED;

    body      = node->child(0);
    condition = node->child(1);
    do{
        result = htree_execute( ctx, frame, body );
        H_FREE_GARBAGE(result);
        H_FREE_GARBAGE(boolean);
    }
    while( (boolean = htree_execute( ctx,  frame, condition ))->lvalue() );

    H_FREE_GARBAGE(result);
    H_FREE_GARBAGE(boolean);

    return H_UNDEFINED;
}

Object *exec_for( h_context_t *ctx, vmem_t *frame, Node *node ){
    Node *condition,
         *increment,
         *body;

    Object *init    = H_UNDEFINED,
           *boolean = H_UNDEFINED,
           *inc     = H_UNDEFINED,
           *result  = H_UNDEFINED;

    init      = htree_execute( ctx,  frame, node->child(0) );
    condition = node->child(1);
    increment = node->child(2);
    body      = node->child(3);
    for( init;
         (boolean = htree_execute( ctx,  frame, condition ))->lvalue();
         (inc     = htree_execute( ctx,  frame, increment )) ){

        result = htree_execute( ctx, frame, body );
        H_FREE_GARBAGE(result);
        H_FREE_GARBAGE(boolean);
        H_FREE_GARBAGE(inc);
    }

    H_FREE_GARBAGE(boolean);
    H_FREE_GARBAGE(inc);
    H_FREE_GARBAGE(init);

    return H_UNDEFINED;
}

Object *exec_foreach( h_context_t *ctx, vmem_t *frame, Node *node ){
    int     i, size;
    Node   *body;
    Object *map    = H_UNDEFINED,
           *result = H_UNDEFINED;
    char   *identifier;

    identifier = (char *)node->child(0)->_identifier.c_str();
    map        = htree_execute( ctx, frame, node->child(1) );
    body       = node->child(2);
    size       = map->xarray.size();

    for( i = 0; i < size; ++i ){
        hybris_vm_add( frame, identifier, map->xarray[i] );
        result = htree_execute( ctx, frame, body );
        H_FREE_GARBAGE(result);
    }

    H_FREE_GARBAGE(map);

    return H_UNDEFINED;
}

Object *exec_foreachm( h_context_t *ctx, vmem_t *frame, Node *node ){
    int     i, size;
    Node   *body;
    Object *map    = H_UNDEFINED,
           *result = H_UNDEFINED;
    char   *key_identifier,
           *value_identifier;

    key_identifier   = (char *)node->child(0)->_identifier.c_str();
    value_identifier = (char *)node->child(1)->_identifier.c_str();
    map              = htree_execute( ctx, frame, node->child(2) );
    body             = node->child(3);
    size             = map->xmap.size();

    for( i = 0; i < size; ++i ){
        hybris_vm_add( frame, key_identifier,   map->xmap[i] );
        hybris_vm_add( frame, value_identifier, map->xarray[i] );
        result = htree_execute( ctx,  frame, body );
        H_FREE_GARBAGE(result);
    }

    H_FREE_GARBAGE(map);

    return H_UNDEFINED;
}

Object *exec_if( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *boolean = H_UNDEFINED,
           *result  = H_UNDEFINED;

    boolean = htree_execute( ctx,  frame, node->child(0) );

    if( boolean->lvalue() ){
        result = htree_execute( ctx, frame, node->child(1) );
    }
    /* handle else case */
    else if( node->children() > 2 ){
        result = htree_execute( ctx, frame, node->child(2) );
    }

    H_FREE_GARBAGE(boolean);
    H_FREE_GARBAGE(result);

    return H_UNDEFINED;
}

Object *exec_question( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *boolean = H_UNDEFINED,
           *result  = H_UNDEFINED;

    boolean = htree_execute( ctx,  frame, node->child(0) );

    if( boolean->lvalue() ){
        result = htree_execute( ctx, frame, node->child(1) );
    }
    else{
        result = htree_execute( ctx, frame, node->child(2) );
    }

    H_FREE_GARBAGE(boolean);

    return result;
}

Object *exec_eostmt( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *res_1 = H_UNDEFINED,
           *res_2 = H_UNDEFINED;

    res_1 = htree_execute( ctx, frame, node->child(0) );
    res_2 = htree_execute( ctx, frame, node->child(1) );

    H_FREE_GARBAGE(res_1);

    return res_2;
}

Object *exec_dot( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a      = H_UNDEFINED,
           *b      = H_UNDEFINED,
           *result = H_UNDEFINED;

    a      = htree_execute( ctx, frame, node->child(0) );
    b      = htree_execute( ctx, frame, node->child(1) );
    result = a->dot( b );

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return result;
}

Object *exec_dote( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a      = H_UNDEFINED,
           *b      = H_UNDEFINED,
           *result = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b      = htree_execute( ctx, frame, node->child(1) );
    result = a->dotequal( b );

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return result;
}

Object *exec_assign( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *object = H_UNDEFINED,
           *value  = H_UNDEFINED;

    value  = htree_execute( ctx,  frame, node->child(1) );
    object = hybris_vm_add( frame, (char *)node->child(0)->_identifier.c_str(), value );

    H_FREE_GARBAGE(value);

    return object;
}

Object *exec_uminus( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o      = H_UNDEFINED,
           *result = H_UNDEFINED;

    o      = htree_execute( ctx, frame, node->child(0) );
    result = -(*o);

    H_FREE_GARBAGE(o);

    return result;
}

Object *exec_regex( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o      = H_UNDEFINED,
           *regexp = H_UNDEFINED,
           *result = H_UNDEFINED;

    o      = htree_execute( ctx,  frame, node->child(0) );
    regexp = htree_execute( ctx,  frame, node->child(1) );
    result = hrex_operator( o, regexp );

    H_FREE_GARBAGE(o);
    H_FREE_GARBAGE(regexp);

    return result;
}

Object *exec_plus( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) + b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_pluse( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) += b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_minus( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) - b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_minuse( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) -= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_mul( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) * b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_mule( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) *= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_div( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) / b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_dive( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) /= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_mod( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) % b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_mode( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) %= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_inc( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED;

    if( (o = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", (char *)node->child(0)->_identifier.c_str() );
    }
    ++(*o);
    return o;
}

Object *exec_dec( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED;

    if( (o = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", (char *)node->child(0)->_identifier.c_str() );
    }
    --(*o);
    return o;
}

Object *exec_xor( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) ^ b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_xore( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) ^= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_and( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) & b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_ande( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) &= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_or( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) | b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_ore( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) |= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_shiftl( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) << b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_shiftle( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) <<= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_shiftr( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) >> b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_shiftre( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED;

    if( (a = hybris_vm_get( frame, (char *)node->child(0)->_identifier.c_str() )) == H_UNDEFINED ){
        hybris_syntax_error( "'%s' undeclared identifier", node->child(0)->_identifier.c_str() );
    }

    b = htree_execute( ctx, frame, node->child(1) );

    (*a) >>= b;

    H_FREE_GARBAGE(b);

    return a;
}

Object *exec_fact( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED,
           *r = H_UNDEFINED;

    o = htree_execute( ctx,  frame, node->child(0) );
    r = o->factorial();

    H_FREE_GARBAGE(o);

    return r;
}

Object *exec_not( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED,
           *r = H_UNDEFINED;

    o = htree_execute( ctx,  frame, node->child(0) );
    r = ~(*o);

    H_FREE_GARBAGE(o);

    return r;
}

Object *exec_lnot( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *o = H_UNDEFINED,
           *r = H_UNDEFINED;

    o = htree_execute( ctx,  frame, node->child(0) );
    r = o->lnot();

    H_FREE_GARBAGE(o);

    return r;
}

Object *exec_less( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) < b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_greater( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) > b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_ge( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) >= b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_le( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) <= b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_ne( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) != b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_eq( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) == b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_land( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) && b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

Object *exec_lor( h_context_t *ctx, vmem_t *frame, Node *node ){
    Object *a = H_UNDEFINED,
           *b = H_UNDEFINED,
           *c = H_UNDEFINED;

    a = htree_execute( ctx, frame, node->child(0) );
    b = htree_execute( ctx, frame, node->child(1) );
    c = (*a) || b;

    H_FREE_GARBAGE(a);
    H_FREE_GARBAGE(b);

    return c;
}

