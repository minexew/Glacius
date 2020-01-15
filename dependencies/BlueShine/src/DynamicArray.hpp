
/*
  BlueShine: Open-Source Database Interface
    Copyright 2009 Minexew

    This file is part of BlueShine.

    BlueShine is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BlueShine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BlueShine.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SHINEARRAY_H__
#define __SHINEARRAY_H__

#include <cstdlib>
#include <cstring>

template <class T>
class DynamicArray
{
    T* data;
    unsigned allocSize;
    
    public:
        DynamicArray( unsigned initSize = 4 ) : allocSize( initSize )
        {
            data = ( T* )malloc( allocSize * sizeof( T ) );
			memset( data, 0, allocSize * sizeof( T ) );
        }
        
        ~DynamicArray()
        {
            free( data );
        }
        
        void resizeTo( unsigned size )
        {
            /* if the requested size is greater than the allocated size,
               resize the array and null the new part */
            if ( allocSize < size )
            {
				unsigned oldSize = allocSize;
                allocSize = size + 4;
                data = ( T* )realloc( data, allocSize * sizeof( T ) );
				memset( data + oldSize * sizeof( T ), 0, ( allocSize - oldSize ) * sizeof( T ) );
            }
        }
        
        T get( unsigned pos )
        {
            if ( pos >= allocSize )
                return NULL;
            return data[ pos ];
        }
        
        void set( unsigned pos, T item )
        {
            resizeTo( pos );
            data[ pos ] = item;
        }
        
        unsigned long add( T item )
        {
            static unsigned pos;
            
            /* first, try to find a "hole" */
            for ( pos = 0; pos < allocSize; pos++ )
                if ( data[ pos ] == NULL )
                    break;

            resizeTo( pos );
            data[ pos ] = item;
            return pos;
        }
};

#endif
