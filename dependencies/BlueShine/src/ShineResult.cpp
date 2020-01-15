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

#include "mysql.hpp"
#include "ShineResult.hpp"

#include <cstdlib>
#include <cstring>

namespace ShineDB
{
    /* Always remember that the real size of QueryOutput::data is ( rows + 1 ) x cols ! */

    QueryOutput::QueryOutput( MYSQL_RES* result )
    {
        /* Store number of rows & columns and alocate the data table */
        rows = mysql_num_rows( result );
        columns = mysql_num_fields( result );
        data = ( char** )malloc( ( rows + 1 ) * columns * sizeof( char* ) );
    
        /* We'll begin by storing the field names */
        MYSQL_FIELD* fields = mysql_fetch_fields( result );
        for ( unsigned col = 0; col < columns; col++ )
        {
            /* Copy the column name into the first row of data table */
            data[ col ] = ( char* )malloc( strlen( fields[ col ].name ) + 1 );
            strcpy( data[ col ], fields[ col ].name );
        }
    
        /* Get the data itself */
        MYSQL_ROW rowData;
        for ( unsigned row = 1; ( rowData = mysql_fetch_row( result ) ) != NULL; row++ )
        {
            /* Fetch field lengths */
            unsigned long* lengths = mysql_fetch_lengths( result );
            /* store field data */
            for ( unsigned col = 0; col < columns; col++ )
            {
                /* allocate + copy */
                data[ row * columns + col ] = ( char* )malloc( lengths[ col ] + 1 );
                memcpy( data[ row * columns + col ], rowData[ col ], lengths[ col ] );
                /* null-terminate */
                data[ row * columns + col ][ lengths[ col ] ] = 0;
            }
        }
    }
    
    QueryOutput::~QueryOutput()
    {
        /* free all items in the table */
        for ( unsigned row = 0; row < rows; row++ )
            for ( unsigned col = 0; col < columns - 1; col++ )
                free( data[ row * columns + col ] );

        /* free the table itself */
        free( data );
    }
}
