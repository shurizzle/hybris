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
#include "node.h"
#include "vmem.h"
#include "engine.h"
#include <assert.h>

NodeValue::NodeValue() :
    m_constant(NULL),
    m_identifier(""),
    m_expression(0),
    m_statement(0),
    m_function(""),
    m_method(""),
    m_access(asPublic),
    m_call(""),
    m_alias_call(NULL),
    m_switch(NULL),
    m_default(NULL) {

}

NodeValue::~NodeValue(){

}

Node::Node() : m_type(H_NT_NONE) {

}

Node::Node( H_NODE_TYPE type ) : m_type(type)
{

}

Node::~Node(){
    int i, sz( size() );

    for( i = 0; i < sz; ++i ){
        Node * child = at(i);
        delete child;
    }
    clear();
}

void Node::addChild( Node *child ){
	push_back(child);
}

Node *Node::clone(){
    int   i, sz( size() );
    Node *clone = H_UNDEFINED;

    switch( m_type ){
        case H_NT_CONSTANT   :

            if( ob_is_int(value.m_constant) ){
                clone = new ConstantNode( (ob_int_ucast(value.m_constant))->value );
            }
            else if( ob_is_float(value.m_constant) ){
                clone = new ConstantNode( ob_float_ucast(value.m_constant)->value );
            }
            else if( ob_is_char(value.m_constant) ){
                clone = new ConstantNode( ob_char_ucast(value.m_constant)->value );
            }
            else if( ob_is_string(value.m_constant) ){
                clone = new ConstantNode( (char *)ob_string_ucast(value.m_constant)->value.c_str() );
            }

        break;

        case H_NT_IDENTIFIER :
            clone = new IdentifierNode( (char *)value.m_identifier.c_str() );
            clone->value.m_access = value.m_access;
        break;

        case H_NT_EXPRESSION   :
            clone = new ExpressionNode( value.m_expression, 0 );
            for( i = 0; i < sz; ++i ){
            	if( child(i) ){
            		clone->push_back( child(i)->clone() );
            	}
            }
        break;

        case H_NT_STATEMENT  :
            clone = new StatementNode( value.m_statement, 0 );
            for( i = 0; i < sz; ++i ){
            	if( child(i) ){
            		clone->push_back( child(i)->clone() );
            	}
            }
        break;

        case H_NT_FUNCTION   :
            clone = new FunctionNode( value.m_function.c_str() );
            for( i = 0; i < sz; ++i ){
            	if( child(i) ){
            		clone->push_back( child(i)->clone() );
            	}
            }
        break;

        case H_NT_CALL       :
            if( value.m_alias_call == NULL ){
                clone = new CallNode( (char *)value.m_call.c_str(), NULL );
            }
            else{
                clone = new CallNode( value.m_alias_call, NULL );
            }
            for( i = 0; i < sz; ++i ){
            	if( child(i) ){
            		clone->push_back( child(i)->clone() );
            	}
            }
        break;

        case H_NT_METHOD :
			clone = new MethodNode( value.m_method.c_str(), value.m_access );
			for( i = 0; i < sz; ++i ){
				if( child(i) ){
					clone->push_back( child(i)->clone() );
				}
			}
        break;

        case H_NT_METHOD_CALL :
			clone = new MethodCallNode( NULL, NULL );
			clone->value.m_method_call = value.m_method_call;
			for( i = 0; i < sz; ++i ){
				if( child(i) ){
					clone->push_back( child(i)->clone() );
				}
			}
        break;

        case H_NT_ATTRIBUTE :
        	clone = new AttributeNode( NULL );
        	clone->value.m_identifier = value.m_identifier;
        	for( i = 0; i < sz; ++i ){
				if( child(i) ){
					clone->push_back( child(i)->clone() );
				}
			}
        break;

        case H_NT_NEW :
        	clone = new NewNode( (char *)value.m_identifier.c_str(), NULL );
        	for( i = 0; i < sz; ++i ){
				if( child(i) ){
					clone->push_back( child(i)->clone() );
				}
			}
        break;

        default:
        	/*
        	 * THIS SHOULD NEVER HAPPEN!
        	 */
        	assert(false);
    }

	return clone;
}

/* constants */
ConstantNode::ConstantNode( long v ) : Node(H_NT_CONSTANT) {
    value.m_constant = (Object *)gc_new_integer(v);
    #ifdef GC_SUPPORT
    value.m_constant->attributes |= H_OA_CONSTANT;
    value.m_constant->attributes &= ~H_OA_GARBAGE;
	#endif
}

ConstantNode::ConstantNode( double v ) : Node(H_NT_CONSTANT) {
    value.m_constant = (Object *)gc_new_float(v);
    #ifdef GC_SUPPORT
    value.m_constant->attributes |= H_OA_CONSTANT;
    value.m_constant->attributes &= ~H_OA_GARBAGE;
	#endif
}

ConstantNode::ConstantNode( char v ) : Node(H_NT_CONSTANT) {
    value.m_constant = (Object *)gc_new_char(v);
    #ifdef GC_SUPPORT
    value.m_constant->attributes |= H_OA_CONSTANT;
    value.m_constant->attributes &= ~H_OA_GARBAGE;
	#endif
}

ConstantNode::ConstantNode( char *v ) : Node(H_NT_CONSTANT) {
    value.m_constant = (Object *)gc_new_string(v);
    #ifdef GC_SUPPORT
    value.m_constant->attributes |= H_OA_CONSTANT;
    value.m_constant->attributes &= ~H_OA_GARBAGE;
	#endif
}

/* expressions */
ExpressionNode::ExpressionNode( int expression ) : Node(H_NT_EXPRESSION) {
    value.m_expression = expression;
}

ExpressionNode::ExpressionNode( int expression, int argc, ... ) : Node(H_NT_EXPRESSION) {
    value.m_expression = expression;
    va_list ap;
	int i;

	reserve(argc);

	va_start( ap, argc );
	for( i = 0; i < argc; ++i ){
		push_back( va_arg( ap, Node * ) );
	}
	va_end(ap);
}

/* statements */
StatementNode::StatementNode( int statement ) : Node(H_NT_STATEMENT) {
    value.m_statement = statement;
}

StatementNode::StatementNode( int statement, int argc, ... ) : Node(H_NT_STATEMENT) {
    value.m_statement = statement;
    va_list ap;
	int i;

    reserve(argc);

	va_start( ap, argc );
	for( i = 0; i < argc; ++i ){
		push_back( va_arg( ap, Node * ) );
	}
	va_end(ap);
}

StatementNode::StatementNode( int statement, Node *sw, NodeList *caselist ) : Node(H_NT_STATEMENT) {
    value.m_statement = statement;
    value.m_switch    = sw;

    if( caselist != NULL ){
        reserve( caselist->size() );
		for( NodeList::iterator ni = caselist->begin(); ni != caselist->end(); ni++ ){
			push_back( *ni );
		}
		delete caselist;
	}
}

StatementNode::StatementNode( int statement, Node *sw, NodeList *caselist, Node *deflt ) : Node(H_NT_STATEMENT) {
    value.m_statement = statement;
    value.m_switch    = sw;
    value.m_default   = deflt;

    if( caselist != NULL ){
        reserve( caselist->size() );
		for( NodeList::iterator ni = caselist->begin(); ni != caselist->end(); ni++ ){
			push_back( *ni );
		}
		delete caselist;
	}
}

/* identifiers */
IdentifierNode::IdentifierNode( char *identifier ) : Node(H_NT_IDENTIFIER) {
    value.m_identifier = identifier;
}

IdentifierNode::IdentifierNode( access_t access, Node *i ) : Node(H_NT_IDENTIFIER) {
	assert( i->type() == H_NT_IDENTIFIER );

	value.m_access     = access;
	value.m_identifier = i->value.m_identifier;
}

IdentifierNode::IdentifierNode( access_t access, char *identifier ) : Node(H_NT_IDENTIFIER) {
    value.m_access     = access;
	value.m_identifier = identifier;
}

/* structure attribute */
AttributeNode::AttributeNode( NodeList *attrlist ) : Node(H_NT_ATTRIBUTE) {
    if( attrlist != NULL ){
        int sz( attrlist->size() );
        NodeList::iterator ni = attrlist->begin();

        value.m_identifier = (*ni)->value.m_identifier;
        reserve( sz - 1 );
        ni++;
		for( ; ni != attrlist->end(); ni++ ){
            push_back( *ni );
		}
		delete attrlist;
	}
}

/* functions */
FunctionNode::FunctionNode( function_decl_t *declaration ) : Node(H_NT_FUNCTION) {
    value.m_function = declaration->function;

    reserve(declaration->argc);

	/* add function prototype args children */
	for( int i = 0; i < declaration->argc; ++i ){
		push_back( new IdentifierNode( declaration->argv[i] ) );
	}
}

FunctionNode::FunctionNode( function_decl_t *declaration, int argc, ... ) : Node(H_NT_FUNCTION) {
    value.m_function = declaration->function;
    va_list ap;
	int i;

    reserve( declaration->argc + argc );

	/* add function prototype args children */
	for( i = 0; i < declaration->argc; ++i ){
		push_back( new IdentifierNode( declaration->argv[i] ) );
	}
	/* add function body statements node */
	va_start( ap, argc );
	for( i = 0; i < argc; ++i ){
		push_back( va_arg( ap, Node * ) );
	}
	va_end(ap);
}

FunctionNode::FunctionNode( const char *name ) : Node(H_NT_FUNCTION) {
    value.m_function = name;
}

/* function calls */
CallNode::CallNode( char *name, NodeList *argv ) : Node(H_NT_CALL) {
    value.m_call = name;
    if( argv != NULL ){
        reserve( argv->size() );
		for( NodeList::iterator ni = argv->begin(); ni != argv->end(); ni++ ){
			push_back( *ni );
		}
		delete argv;
	}
}

CallNode::CallNode( Node *alias, NodeList *argv ) :  Node(H_NT_CALL) {
    value.m_alias_call = alias;
    if( argv != NULL ){
        reserve( argv->size() );
		for( NodeList::iterator ni = argv->begin(); ni != argv->end(); ni++ ){
			push_back( *ni );
		}
		delete argv;
	}
}

/* structure or class creation */
NewNode::NewNode( char *type, NodeList *argv ) : Node(H_NT_NEW){
	value.m_identifier = type;
	if( argv != NULL ){
		reserve( argv->size() );
		for( NodeList::iterator ni = argv->begin(); ni != argv->end(); ni++ ){
			push_back( *ni );
		}
		delete argv;
	}
}

/* struct type definition */
StructureNode::StructureNode( char *s_name, NodeList *attributes ) : Node(H_NT_STRUCT) {
    value.m_identifier = s_name;
    if( attributes != NULL ){
        reserve( attributes->size() );
		for( NodeList::iterator ni = attributes->begin(); ni != attributes->end(); ni++ ){
			push_back( *ni );
		}
		delete attributes;
	}
}

/* methods */
MethodNode::MethodNode( access_t access, method_decl_t *declaration, int argc, ... ) : Node(H_NT_METHOD) {
    value.m_method = declaration->method;
    value.m_access = access;

    va_list ap;
	int i;

    reserve( declaration->argc + argc );
	/* add method prototype args children */
	for( i = 0; i < declaration->argc; ++i ){
		push_back( new IdentifierNode( declaration->argv[i] ) );
	}
	/* add method body statements node */
	va_start( ap, argc );
	for( i = 0; i < argc; ++i ){
		push_back( va_arg( ap, Node * ) );
	}
	va_end(ap);
}

MethodNode::MethodNode( const char *name, access_t access ) : Node(H_NT_METHOD) {
	value.m_method = name;
	value.m_access = access;
}

/* class type definition */
ClassNode::ClassNode( char *classname, NodeList *extends, NodeList *members ) : Node(H_NT_CLASS) {
	value.m_identifier = classname;
	if( extends != NULL ){
		for( NodeList::iterator ni = extends->begin(); ni != extends->end(); ni++ ){
			m_extends.push_back( *ni );
		}
	}
	if( members != NULL ){
		reserve( members->size() );
		for( NodeList::iterator ni = members->begin(); ni != members->end(); ni++ ){
			push_back( *ni );
		}
		delete members;
	}
}

/* method calls (a subset of StatementNode) */
MethodCallNode::MethodCallNode( NodeList *mcall, NodeList *argv ) : Node(H_NT_METHOD_CALL) {
	if( mcall ){
		value.m_method_call = *mcall;
		delete mcall;
	}
	if( argv != NULL ){
		reserve( argv->size() );
		for( NodeList::iterator ni = argv->begin(); ni != argv->end(); ni++ ){
			push_back( *ni );
		}
		delete argv;
	}
}
