
#include "Config.hpp"
#include "Database.hpp"

//#define Driver_MySQL_mysql
#define Driver_SQLite_sqlite

#ifdef Driver_MySQL_mysql
#include <BlueShine.h>
#endif

namespace Glacius
{
    Database* dbGlobal = 0;

    inline String escape( String input )
    {
        return input.replaceAll( "'", "''" );
    }

    inline unsigned hashPassword( const char* pass )
    {
        unsigned hash = 100;

        if ( !pass )
            return 0;

        for ( int i = 0; pass[i]; i++ )
            hash += pass[i] * ( i + 1 );

        //printf( "'%s' -> %u\n", pass, hash );
        return hash;
    }

    // *********************************************************************
    //  Glacius::Database
    // *********************************************************************

    Database::~Database()
    {
    }

    Database* Database::create()
    {
        String protocol = confGlobal->getOption( "Database.protocol" );

#ifdef Driver_MySQL_mysql
        if ( protocol == "mysql" )
            return new MySqlDatabase();
        else
#endif
#ifdef Driver_SQLite_sqlite
        if ( protocol == "sqlite" )
            return new SqliteDatabase();
        else
#endif
            throw Exception( "Glacius.Database.UnknownProtocol", "Unknown database protocol " + protocol );
    }

    // *********************************************************************
    //  Glacius::MySqlDatabase
    // *********************************************************************
#ifdef Driver_MySQL_mysql

    MySqlDatabase::MySqlDatabase( cfx2::Node& options )
    {
        const char* host, * username, * pass, * db;

        if ( options.

        if ( cfx2_get_node_attrib( options, "host", &host ) )
            throw Exception( "Glacius.MySqlDatabase.MissingOption", "Missing option 'host'" );

        if ( cfx2_get_node_attrib( options, "username", &username ) )
            throw Exception( "Glacius.MySqlDatabase.MissingOption", "Missing option 'username'" );

        if ( cfx2_get_node_attrib( options, "pass", &pass ) )
            throw Exception( "Glacius.MySqlDatabase.MissingOption", "Missing option 'pass'" );

        if ( cfx2_get_node_attrib( options, "db", &db ) )
            throw Exception( "Glacius.MySqlDatabase.MissingOption", "Missing option 'db'" );

        if ( !BS_connect( host, username, pass, db ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        printf( "[MySqlDatabase] Using database `%s` @ `%s`\n", db, host );
    }

    MySqlDatabase::~MySqlDatabase()
    {
        BS_close( 0 );
    }

    int MySqlDatabase::executeCommand( const char* text )
    {
        return ( int )BS_query( 0, text );
    }

    String MySqlDatabase::getConfigOption( const char* name )
    {
        String query = ( String )"SELECT Value FROM RealmConfig WHERE `Option` = '" + escape( name ) + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_num_rows( 0 ) < 1 )
            throw Exception( "Glacius.MySqlDatabase.RealmConfigError", ( String ) "Missing configuration option '" + name + "'" );

        return BS_get_cell_by_index( 0, 0, 0 );
    }

    void MySqlDatabase::setConfigOption( const char* name, const char* value )
    {
        String query = ( String )"UPDATE RealmConfig SET Value = '" + escape( value ) + "' WHERE Option = '" + escape( name ) + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_affected_rows( 0 ) > 0 )
            // the entry had already existed and was successfully updated
            return;

        query = ( String )"INSERT INTO RealmConfig (Option, Value) VALUES ('" + escape( name ) + "', '" + escape( value ) + "')";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );
    }

    int MySqlDatabase::createAccount( const char* name, const char* password, DbOpResult& result )
    {
        String query = ( String )"SELECT AccountID FROM Accounts WHERE Name = '" + escape( name ) + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_num_rows( 0 ) > 0 )
        {
            result.errorName = "result.err_account_exists";
            return -1;
        }

        query = ( String )"INSERT INTO Accounts (Name, Pass) VALUES ('" + escape( name ) + "', MD5('" + escape( password ) + "'))";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        return ( int )BS_last_insert_id( 0 );
    }

    int MySqlDatabase::login( const char* name, const char* password, DbOpResult& result )
    {
        String query = ( String )"SELECT AccountID FROM Accounts WHERE Name = '" + escape( name ) + "' AND Pass = MD5('" + password + "')";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_num_rows( 0 ) == 0 )
        {
            result.errorName = "result.err_login_incorrect";
            return -1;
        }

        return strtol( BS_get_cell_by_index( 0, 0, 0 ), 0, 0 );
    }

    int MySqlDatabase::createCharacter( int accountID, CharacterCreationInfo& input, DbOpResult& result )
    {
        String query = ( String )"SELECT CharID FROM Characters WHERE AccountID = '" + accountID + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_num_rows( 0 ) >= 5 )
        {
            result.errorName = "result.err_max_characters_reached";
            return -1;
        }

        query = ( String )"SELECT CharID FROM Characters WHERE Name = '" + escape( input.name ) + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        if ( BS_num_rows( 0 ) > 0 )
        {
            result.errorName = "result.err_character_exists";
            return -1;
        }

        query = ( String )"INSERT INTO Characters (AccountID, Name, Race)"
                " VALUES ('" + accountID + "', '" + escape( input.name ) + "', '" + input.race + "')";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        return ( int )BS_last_insert_id( 0 );
    }

    int MySqlDatabase::getCharacterList( int accountID, CharacterSummary* output )
    {
        String query = ( String )"SELECT CharID, Name, Race, Class, Level, AreaName, ZoneName"
                " FROM Characters LEFT JOIN ZoneNames ON ZoneNames.Zone = Characters.Zone WHERE AccountID = '" + accountID + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        int count = ( int )BS_num_rows( 0 );

        if ( count < 0 || count > 5 )
            throw Exception( "Glacius.MySqlDatabase.CharacterCountError", ( String )count + " not in range 0 - 5" );

        for ( int i = 0; i < count; i++ )
        {
            output[i].charID = strtol( BS_get_cell_by_index( 0, i, 0 ), 0, 0 );
            output[i].name = BS_get_cell_by_index( 0, i, 1 );
            output[i].race = strtol( BS_get_cell_by_index( 0, i, 2 ), 0, 0 );
            output[i].classID = strtol( BS_get_cell_by_index( 0, i, 3 ), 0, 0 );
            output[i].level = strtol( BS_get_cell_by_index( 0, i, 4 ), 0, 0 );
            output[i].area = BS_get_cell_by_index( 0, i, 5 );
            output[i].zone = BS_get_cell_by_index( 0, i, 6 );
        }

        return count;
    }

    int MySqlDatabase::loadCharacter( int charID, CharacterProperties* output )
    {
        String query = ( String )"SELECT CharID, Name, Race, Class, Level, AreaName, ZoneName, Zone, x, y, z, orientation, Gold"
                " FROM Characters LEFT JOIN ZoneNames ON ZoneNames.Zone = Characters.Zone WHERE CharID = '" + charID + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        int count = ( int )BS_num_rows( 0 );

        if ( count != 1 )
            throw Exception( "Glacius.MySqlDatabase.CharacterLoadingError", ( String )"expected 1 character, got " + count );

        output->charID = strtol( BS_get_cell_by_index( 0, 0, 0 ), 0, 0 );
        output->name = BS_get_cell_by_index( 0, 0, 1 );
        output->race = strtol( BS_get_cell_by_index( 0, 0, 2 ), 0, 0 );
        output->classID = strtol( BS_get_cell_by_index( 0, 0, 3 ), 0, 0 );
        output->level = strtol( BS_get_cell_by_index( 0, 0, 4 ), 0, 0 );
        output->area = BS_get_cell_by_index( 0, 0, 5 );
        output->zone = BS_get_cell_by_index( 0, 0, 6 );
        output->zoneID = strtol( BS_get_cell_by_index( 0, 0, 7 ), 0, 0 );
        output->x = ( float )strtod( BS_get_cell_by_index( 0, 0, 8 ), 0 );
        output->y = ( float )strtod( BS_get_cell_by_index( 0, 0, 9 ), 0 );
        output->z = ( float )strtod( BS_get_cell_by_index( 0, 0, 10 ), 0 );
        output->orientation = ( float )strtod( BS_get_cell_by_index( 0, 0, 11 ), 0 );
        output->gold = strtol( BS_get_cell_by_index( 0, 0, 12 ), 0, 0 );

        return 1;
    }

    int MySqlDatabase::saveCharacter( CharacterProperties* input )
    {
        String query = ( String )"UPDATE Characters SET x = '" + input->x + "', y = '" + input->y + "', z = '" + input->z + "', orientation = '" + input->orientation + "'"
                " WHERE CharID = '" + input->charID + "'";

        if ( !BS_query( 0, query ) )
            throw Exception( "Glacius.MySqlDatabase.DbError", BS_error() );

        return 1;
    }

#endif
    // *********************************************************************
    //  Glacius::SqliteDatabase
    // *********************************************************************

    SqliteDatabase::SqliteDatabase() : db( 0 )
    {
        String fileName = confGlobal->getOption( "Database/fileName" );

        if ( sqlite3_open( fileName, &db ) )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", sqlite3_errmsg( db ) );
            sqlite3_close( db );
            throw ex;
        }

        Console::writeLine( "[SqliteDatabase] Using database file " + File::formatFileName( fileName ) );
    }

    SqliteDatabase::~SqliteDatabase()
    {
        sqlite3_close( db );
    }

    int SqliteDatabase::executeCommand( const char* text )
    {
        char* error;

        if ( sqlite3_exec( db, text, 0, 0, &error ) == SQLITE_OK )
            return true;
        else
        {
            printf( "[SqliteDatabase] Warning: executeCommand() failed:\n\t%s\n\n", error );
            sqlite3_free( error );
            return false;
        }
    }

    void SqliteDatabase::requireQuery( const char* query, int ( *callback )( void*, int, char**, char** ), void* param )
    {
        char* error = 0;

        int queryResult = sqlite3_exec( db, query, callback, param, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
            sqlite3_free( error );
    }

    struct ConfigOptionQuery
    {
        String value;
        bool found;
    };

    static int getConfigOptionCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "getConfigOptionCallback(): %i columns, %s = %s\n", numCols, names[0], columns[0] );

        if ( numCols > 0 && columns[0] )
        {
            ConfigOptionQuery* query = ( ConfigOptionQuery* )param;
            query->value = columns[0];
            query->found = true;
            return 1;
        }

        return 0;
    }

    String SqliteDatabase::getConfigOption( const char* name )
    {
        String query = ( String )"SELECT Value FROM RealmConfig WHERE Option = '" + escape( name ) + "'";

        ConfigOptionQuery queryData;
        queryData.found = false;

        requireQuery( query, getConfigOptionCallback, &queryData );

        if ( !queryData.found )
            throw Exception( "Glacius.SqliteDatabase.RealmConfigError", ( String ) "Missing configuration option '" + name + "'" );

        return queryData.value;
    }

    void SqliteDatabase::setConfigOption( const char* name, const char* value )
    {
        String query = ( String )"UPDATE RealmConfig SET Value = '" + escape( value ) + "' WHERE Option = '" + escape( name ) + "'";

        requireQuery( query );

        if ( sqlite3_changes( db ) > 0 )
            // the entry had already existed and was successfully updated
            return;

        query = ( String )"INSERT INTO RealmConfig (Option, Value) VALUES ('" + escape( name ) + "', '" + escape( value ) + "')";

        requireQuery( query );
    }

    struct AccountSearch
    {
        int accountID;
        bool found;
    };

    static int accountSearchCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "accountSearchCallback(): %i columns, %s = %s\n", numCols, names[0], columns[0] );

        if ( numCols > 0 && columns[0] )
        {
            AccountSearch* search = ( AccountSearch* )param;
            search->accountID = strtol( columns[0], 0, 0 );
            search->found = true;
            return 1;
        }

        return 0;
    }

    int SqliteDatabase::createAccount( const char* name, const char* password, DbOpResult& result )
    {
        String query = ( String )"SELECT AccountID FROM Accounts WHERE Name = '" + escape( name ) + "'";

        AccountSearch searchData;
        searchData.found = false;

        char* error = 0;

        int queryResult = sqlite3_exec( db, query, accountSearchCallback, &searchData, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( searchData.found )
        {
            result.errorName = "result.err_account_exists";
            return -1;
        }

        query = ( String )"INSERT INTO Accounts (Name, Pass) VALUES ('" + name + "', '" + hashPassword( password ) + "')";

        if ( sqlite3_exec( db, query, 0, 0, &error ) != SQLITE_OK )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }

        return ( int )sqlite3_last_insert_rowid( db );
    }

    int SqliteDatabase::login( const char* name, const char* password, DbOpResult& result )
    {
        String query = ( String )"SELECT AccountID FROM Accounts WHERE Name = '" + escape( name ) + "' AND Pass = '" + hashPassword( password ) + "'";

        AccountSearch searchData;
        searchData.found = false;

        char* error = 0;

        int queryResult = sqlite3_exec( db, query, accountSearchCallback, &searchData, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( !searchData.found )
        {
            result.errorName = "result.err_login_incorrect";
            return -1;
        }

        return searchData.accountID;
    }

    struct CharacterIteration
    {
        int numFound;
    };

    struct CharacterSearch
    {
        bool found;
    };

    static int characterIterationCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "characterIterationCallback(): %i columns, %s = %s\n", numCols, names[0], columns[0] );

        CharacterIteration* iter = ( CharacterIteration* )param;
        iter->numFound++;

        return 0;
    }

    static int characterSearchCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "characterSearchCallback(): %i columns, %s = %s\n", numCols, names[0], columns[0] );

        CharacterSearch* search = ( CharacterSearch* )param;
        search->found = true;
        return 1;
    }

    int SqliteDatabase::createCharacter( int accountID, CharacterCreationInfo& input, DbOpResult& result )
    {
        String query = ( String )"SELECT CharID FROM Characters WHERE AccountID = '" + accountID + "'";

        CharacterIteration iterInfo;
        iterInfo.numFound = 0;

        char* error = 0;

        int queryResult = sqlite3_exec( db, query, characterIterationCallback, &iterInfo, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( iterInfo.numFound >= 5 )
        {
            result.errorName = "result.err_max_characters_reached";
            return -1;
        }

        query = ( String )"SELECT CharID FROM Characters WHERE Name = '" + escape( input.name ) + "'";

        CharacterSearch searchInfo;
        searchInfo.found = false;

        queryResult = sqlite3_exec( db, query, characterSearchCallback, &searchInfo, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( searchInfo.found )
        {
            result.errorName = "result.err_character_exists";
            return -1;
        }

        query = ( String )"INSERT INTO Characters (AccountID, Name, Race)"
                " VALUES ('" + accountID + "', '" + escape( input.name ) + "', '" + input.race + "')";

        if ( sqlite3_exec( db, query, 0, 0, &error ) != SQLITE_OK )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }

        return ( int )sqlite3_last_insert_rowid( db );
    }

    struct CharacterListing
    {
        CharacterSummary* output;
        int numFound;
    };

    static int characterListCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "characterListCallback(): %i columns\n", numCols );

        CharacterListing* list = ( CharacterListing* )param;
        int i = list->numFound++;

        list->output[i].charID = strtol( columns[0], 0, 0 );
        list->output[i].name = columns[1];
        list->output[i].race = strtol( columns[2], 0, 0 );
        list->output[i].classID = strtol( columns[3], 0, 0 );
        list->output[i].level = strtol( columns[4], 0, 0 );
        list->output[i].area = columns[5];
        list->output[i].zone = columns[6];

        return 0;
    }

    int SqliteDatabase::getCharacterList( int accountID, CharacterSummary* output )
    {
        String query = ( String )"SELECT CharID, Name, Race, Class, Level, AreaName, ZoneName"
                " FROM Characters LEFT JOIN ZoneNames ON ZoneNames.Zone = Characters.Zone WHERE AccountID = '" + accountID + "'";

        CharacterListing listInfo;
        listInfo.output = output;
        listInfo.numFound = 0;

        char* error = 0;

        int queryResult = sqlite3_exec( db, query, characterListCallback, &listInfo, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( listInfo.numFound > 5 )
            throw Exception( "Glacius.SqliteDatabase.CharacterCountError", ( String )listInfo.numFound + " not in range 0 - 5" );

        return listInfo.numFound;
    }

    struct CharacterLoad
    {
        CharacterProperties* output;
        bool found;
    };

    static int characterLoadCallback( void* param, int numCols, char** columns, char** names )
    {
        //printf( "characterLoadCallback(): %i columns\n", numCols );

        CharacterLoad* character = ( CharacterLoad* )param;

        character->output->charID = strtol( columns[0], 0, 0 );
        character->output->name = columns[1];
        character->output->race = strtol( columns[2], 0, 0 );
        character->output->classID = strtol( columns[3], 0, 0 );
        character->output->level = strtol( columns[4], 0, 0 );
        character->output->area = columns[5];
        character->output->zone = columns[6];
        character->output->zoneID = strtol( columns[7], 0, 0 );
        character->output->x = ( float )strtod( columns[8], 0 );
        character->output->y = ( float )strtod( columns[9], 0 );
        character->output->z = ( float )strtod( columns[10], 0 );
        character->output->orientation = ( float )strtod( columns[11], 0 );
        character->output->gold = strtol( columns[12], 0, 0 );
        character->found = true;

        return 0;
    }

    int SqliteDatabase::loadCharacter( int charID, CharacterProperties* output )
    {
        String query = ( String )"SELECT CharID, Name, Race, Class, Level, AreaName, ZoneNames.ZoneName, ZoneNames.Zone, x, y, z, orientation, Gold"
                " FROM Characters LEFT JOIN ZoneNames ON ZoneNames.Zone = Characters.Zone WHERE CharID = '" + charID + "'";

        CharacterLoad charInfo;
        charInfo.output = output;
        charInfo.found = false;

        char* error = 0;

        int queryResult = sqlite3_exec( db, query, characterLoadCallback, &charInfo, &error );

        if ( queryResult != SQLITE_OK && queryResult != SQLITE_ABORT )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }
        else if ( error )
        {
            sqlite3_free( error );
            error = 0;
        }

        if ( !charInfo.found )
            throw Exception( "Glacius.SqliteDatabase.CharacterLoadingError", ( String )"character not found (charID = " + charID + ")" );

        return 1;
    }

    int SqliteDatabase::saveCharacter( CharacterProperties* input )
    {
        String query = ( String )"UPDATE Characters SET x = '" + input->x + "', y = '" + input->y + "', z = '" + input->z + "', orientation = '" + input->orientation + "'"
                " WHERE CharID = '" + input->charID + "'";

        char* error;

        if ( sqlite3_exec( db, query, 0, 0, &error ) != SQLITE_OK )
        {
            Exception ex = Exception( "Glacius.SqliteDatabase.DbError", error );
            sqlite3_free( error );
            throw ex;
        }

        return 1;
    }
}
