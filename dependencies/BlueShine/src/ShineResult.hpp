#pragma once

/*
  BlueShine: Open-Source Database Interface
    Copyright 2009, 2010 Minexew

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

#ifndef __mysql_hpp__
struct MYSQL_RES;
#endif

namespace ShineDB
{
    /* Always remember that the real size of QueryOutput::data is ( rows + 1 ) x cols ! */

    struct QueryOutput
    {
        unsigned rows, columns;
        char** data;
    
        public:
            QueryOutput( MYSQL_RES* result );
            ~QueryOutput();
            
            const char* title( unsigned col )
            {
                return data[ col ];
            }
            
            const char* get( unsigned row, unsigned col )
            {
                return data[ ( row + 1 ) * columns + col ];
            }
    };
}
