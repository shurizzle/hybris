/*
 * This file is part of the Hybris programming language.
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
import  std.os.dll;
include std.Exception;

class Dll {
	protected name, link;

	public method Dll( name ){
		me->name = name;
		me->link = dllopen( me->name );
		
		if( !me->link ){
			throw new Exception( __FILE__, __LINE__, "Could not open ".me->name." dynamic library." );
		}
	}

	public method __expire(){
		me->close();	
	}

	public method close(){
		dllclose( me->link );
	}
	
	public method sym( name ){
		return dlllink( me->link, name );
	}

	operator [] ( name ){
		return me->sym(name);
	}
}
