
#pragma once

#include <sqlite3.h>

#include <littl.hpp>
#include <littl/cfx2.hpp>

namespace Glacius
{
    using namespace li;

    class Database;

    extern Database* dbGlobal;

    struct DbOpResult
    {
        String errorName;
    };

    struct CharacterCreationInfo
    {
        String name;
        int race;
    };

    struct CharacterSummary
    {
        String name, area, zone;
        int charID, level, race, classID;
    };

    struct CharacterProperties
    {
        String name, area, zone;
        int charID, level, race, classID, zoneID, gold;
        float x, y, z, orientation;
    };

    class Database
    {
        public:
            virtual ~Database() = 0;

            static Database* create();

            virtual int executeCommand( const char* text ) = 0;
            virtual String getConfigOption( const char* name ) = 0;
            virtual void setConfigOption( const char* name, const char* value ) = 0;

            virtual int createAccount( const char* name, const char* password, DbOpResult& result ) = 0;
            virtual int login( const char* name, const char* password, DbOpResult& result ) = 0;

            virtual int createCharacter( int accountID, CharacterCreationInfo& input, DbOpResult& result ) = 0;
            virtual int getCharacterList( int accountID, CharacterSummary* output ) = 0;
            virtual int loadCharacter( int charID, CharacterProperties* output ) = 0;
            virtual int saveCharacter( CharacterProperties* output ) = 0;
    };

    class MySqlDatabase : public Database
    {
        public:
            MySqlDatabase();
            virtual ~MySqlDatabase();

            virtual int executeCommand( const char* text );
            virtual String getConfigOption( const char* name );
            virtual void setConfigOption( const char* name, const char* value );

            virtual int createAccount( const char* name, const char* password, DbOpResult& result );
            virtual int login( const char* name, const char* password, DbOpResult& result );

            virtual int createCharacter( int accountID, CharacterCreationInfo& input, DbOpResult& result );
            virtual int getCharacterList( int accountID, CharacterSummary* output );
            virtual int loadCharacter( int charID, CharacterProperties* output );
            virtual int saveCharacter( CharacterProperties* output );
    };

    class SqliteDatabase : public Database
    {
        sqlite3* db;

        void requireQuery( const char* query, int ( *callback )( void*, int, char**, char** ) = 0, void* param = 0 );

        public:
            SqliteDatabase();
            virtual ~SqliteDatabase();

            virtual int executeCommand( const char* text );
            virtual String getConfigOption( const char* name );
            virtual void setConfigOption( const char* name, const char* value );

            virtual int createAccount( const char* name, const char* password, DbOpResult& result );
            virtual int login( const char* name, const char* password, DbOpResult& result );

            virtual int createCharacter( int accountID, CharacterCreationInfo& input, DbOpResult& result );
            virtual int getCharacterList( int accountID, CharacterSummary* output );
            virtual int loadCharacter( int charID, CharacterProperties* output );
            virtual int saveCharacter( CharacterProperties* output );
    };
}
