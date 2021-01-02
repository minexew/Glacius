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
#include "ShineDB.hpp"

#include <cstdlib>
#include <cstring>

#define INTERNAL_ERROR( err ) ( "ShineDB error: internal error ( " err " ) // " __FILE__ )

namespace ShineDB
{
    static char* lastError = 0;
    
    const char* getLastError()
    {
        return lastError;
    }
    
    void setLastError( const char* errorString )
    {
        if ( lastError )
            free( lastError );
        
        lastError = ( char* )malloc( strlen( errorString ) + 1 );
        strcpy( lastError, errorString );
    }

    Session::Session( MYSQL* sess ) : mySQL( sess )
    {
        affectedRows = 0;
    }
    
    Session::~Session()
    {
        mysql_close( mySQL );
    }
    
    Session* Session::create( const char* host, const char* username, const char* password, const char* dbName )
    {
        setLastError( INTERNAL_ERROR( "interrupted - Session::create" ) );

        /* Allocate the MySQL struct */
        MYSQL* mySQL = mysql_init( 0 );
        
        /* Now, let's connect to the server... */
        if ( !mysql_real_connect( mySQL, host, username, password, dbName, 0, 0, CLIENT_MULTI_STATEMENTS ) )
        {
            setLastError( mysql_error( mySQL ) );
            mysql_close( mySQL );
            return 0;
        }
        
        // MySQL 8 removes `my_bool`.
        // According to https://github.com/Motion-Project/motion/pull/1038/files, it should be ok to use an int
        int reconnect = 1;
        mysql_options( mySQL, MYSQL_OPT_RECONNECT, &reconnect );
        
        /* Finally, create the session struct */
		setLastError( "" );
        return new Session( mySQL );
    }
    
    bool Session::query( const char* query, QueryOutput*& output )
    {
        setLastError( INTERNAL_ERROR( "interrupted - Session::query" ) );

        /* Reset results */
        affectedRows = 0;

        /* Check if everything is OK */
        if ( !mySQL || !query )
        {
            setLastError( INTERNAL_ERROR( "null pointer - Session::query" ) );
            return false;
        }

        /* Do the query! */
        if ( mysql_query( mySQL, query ) )
        {
            setLastError( mysql_error( mySQL ) );
            return false;
        }

        /* Get the results */
        int more;
        MYSQL_RES* result;

        do
        {
            /* did current statement return data? */
            result = mysql_store_result( mySQL );
            if ( result )
            {
                /* yes, we've got some results */
                output = new QueryOutput( result );
                mysql_free_result( result );
            }
            else
            {
                /* no results or an error */
                if ( mysql_errno( mySQL ) == 0 ) /* alternative is to use mysql_field_count */
                {
                    affectedRows += mysql_affected_rows( mySQL );
                    output = 0;
                }
                else  /* some error occurred */
                {
                    setLastError( mysql_error( mySQL ) );
                    return false;
                }
            }

            /* any more results? ( > 0 means error ) */
            if ( ( more = mysql_next_result( mySQL ) ) > 0 )
            {
                setLastError( mysql_error( mySQL ) );
                return false;
            }
        }
        while ( more == 0 );

        setLastError( "" );
        return true;
    }

    unsigned Session::lastInsertID()
    {
        return mysql_insert_id( mySQL );
    }
}
