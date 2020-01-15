
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

#ifdef _WIN32
#define BlueShine extern "C" __declspec( dllexport )
#else
#define BlueShine
#endif

#include "mysql.hpp"

#include "ShineDB.hpp"
#include "DynamicArray.hpp"
#include "IdHandling.hpp"

#include <cstring>

DynamicArray<ShineDB::Session*> sessions;
DynamicArray<ShineDB::QueryOutput*> results;

ShineDB::QueryOutput* lastOutput = 0;
unsigned long lastSession = 0;

/* ******************************** */

BlueShine double BS_connect( const char* host, const char* user, const char* pass, const char* db )
{
	ShineDB::Session* newSession = ShineDB::Session::create( host, user, pass, db );
    if ( newSession == 0 )
        return 0;
    else
        return lastSession = sessions.add( newSession ) + 1;
}

BlueShine const char* BS_error()
{
	return ShineDB::getLastError();
}

BlueShine double BS_close( double sid )
{
    validateSid();
    delete session;
    sessions.set( sessId - 1, 0 );
	return 1;
}

BlueShine double BS_query( double sid, const char* query )
{
    validateSid();
    if ( lastOutput != NULL )
        delete lastOutput;
    return session->query( query, lastOutput );
}

BlueShine double BS_affected_rows( double sid )
{
    validateSid();
    return session->getAffectedRows();
}

BlueShine double BS_last_insert_id( double sid )
{
    validateSid();
    return session->lastInsertID();
}

BlueShine double BS_store_result()
{
    unsigned long resultId = results.add( lastOutput );
    lastOutput = NULL;
    return resultId + 1;
}

BlueShine double BS_num_fields( double rid )
{
    validateRid( -1 );
    return result->columns;
}

BlueShine const char* BS_get_field( double rid, double index )
{
    validateRid( "" );
    unsigned long col = ( unsigned long )index;
    if ( col < result->columns )
        return result->title( col );
    return "";
}

BlueShine double BS_num_rows( double rid )
{
    validateRid( -1 );
    return result->rows;
}

BlueShine const char* BS_get_cell_by_name( double rid, double r, const char* name )
{
    validateRid( "" );
    unsigned long row = ( unsigned long )r;
    if ( row < result->rows )
    {
        for ( unsigned long col = 0; col < result->columns; col++ )
            if ( strcmp( result->title( col ), name ) == 0 )
                return result->get( ( unsigned long )row, col );
    }
    return "";
}

BlueShine const char* BS_get_cell_by_index( double rid, double r, double index )
{
    validateRid( "" );
    unsigned long col = ( unsigned long )index;
    unsigned long row = ( unsigned long )r;
    if ( row < result->rows && col < result->columns )
        return result->get( row, col );
    return "";
}

BlueShine double BS_free_result( double rid )
{
    validateRid( false );
    delete result;

    if ( resultId > 0 )
        results.set( resultId - 1, NULL );
    else
        lastOutput = NULL;
	return true;
}

/* ******************************** */

#ifdef _WIN32
BOOL APIENTRY DllMain( HINSTANCE hInst, DWORD reason, void* reserved )
{
	hInst = hInst;
	reserved = reserved;

    switch ( reason )
    {
        case DLL_PROCESS_ATTACH:
            /*MessageBox( NULL, "Attached!", "BlueShine", MB_ICONINFORMATION );*/
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return true;
}
#endif
