
#if defined( _DEBUG ) && defined( _MSC_VER )
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <littl.hpp>

#include "Config.hpp"
#include "LoginServer.hpp"
#include "StatusServer.hpp"
#include "WorldServer.hpp"

using namespace Glacius;
using namespace li;

static Console console;
static bool restart;

static List<String> playerList;
static Mutex syncMutex;

static void syncPlayer( unsigned pid, const String& name, unsigned interval, float x, float y )
{
    syncMutex.enter();
    playerList.add( ( String )"[" + pid + "] '" + name + "' sync delta = " + interval + " ms; world coords: " + x + ", " + y );
    syncMutex.leave();
}

static void run( const String& configFile )
{
    try
    {
        printf( "## reading configuration file: %s\n", configFile.c_str() );
        confGlobal = new Config( configFile );

        printf( "## creating database session...\n" );
        dbGlobal = Database::create();

        printf( "## starting Login Server\n" );
        loginGlobal = new LoginServer();
        loginGlobal->start();
        pauseThread( 200 );

        printf( "## starting World Server\n" );
        worldGlobal = new WorldServer();
        worldGlobal->start();
        pauseThread( 200 );

        StatusServer* status = 0;

        if ( confGlobal->getOption( "StatusServer/enabled", true ).toInt() )
        {
            printf( "## starting Status Server\n" );
            status = new StatusServer();
            status->start();
            pauseThread( 200 );
        }

        if ( confGlobal->getOption( "Server/isConfigured", true ).isEmpty() )
        {
            loginGlobal->setStatus( true, "Initial configuration." );

            console.write( "\n\n-------------------------------------------------------------------------------\n" );
            console.writeLine( "## Welcome to Glacius!" );
            console.writeLine( "##" );
            console.writeLine( "## It seems this is the first time the server is run." );
            console.writeLine( "## All incoming connections to the server have been automatically disabled." );
            console.writeLine( "## You can now import your database and configure server options." );
            console.writeLine( "## When you are done, just type \"up\" in the server console and incoming" );
            console.writeLine( "## connections will be re-enabled." );
            console.writeLine( "##" );
            console.writeLine( "## Press enter to start!" );
            console.readLine();

            confGlobal->setOption( "Server/isConfigured", "1" );
        }

        printf( "\n\n-------------------------------------------------------------------------------\n" );
        printf( "## The server is now running. Type \"help\" for a list of server commands.\n" );
        printf( "## To stop the server, type \"exit\".\n" );
        printf( "-------------------------------------------------------------------------------\n\n" );
        printf( "\n" );

        while ( true )
        {
            List<String> tokens;

            printf( "[glacius@localhost]> " );
            String command = console.readLine();

            if ( command.isEmpty() )
                continue;

            command.parse( tokens, ' ', '\\' );

            iterate ( tokens )
                if ( tokens.current() == "$" )
                    tokens.current() = console.readLine();

            if ( tokens[0] == "down" )
            {
                loginGlobal->setStatus( true, tokens[1] );
                console.writeLine( " -- The server is now down for maintenance." );
            }
            else if ( tokens[0] == "exit" )
            {
                break;
            }
            else if ( tokens[0] == "get" )
            {
                for ( size_t i = 1; i < tokens.getLength(); i++ )
                {
                    try
                    {
                        String value = dbGlobal->getConfigOption( tokens[i] );
                        printf( "%s = '%s'\n", tokens[i].c_str(), value.c_str() );
                    }
                    catch ( Exception ex )
                    {
                        printf( "%s = undefined\n", tokens[i].c_str() );
                    }
                }
            }
            else if ( tokens[0] == "help" )
            {
                String commands = File::read( "commands" );

                if ( commands.isEmpty() )
                {
                    console.writeLine();
                    console.writeLine( " -- Please see the file 'commands' for the list of available commands." );
                    console.writeLine();
                }
                else
                    Console::writeLine( commands );
            }
            else if ( tokens[0] == "import" )
            {
                String script = File::read( tokens[1] );

                if ( script.isEmpty() )
                    console.writeLine( " -- Failed to load '" + tokens[1] + "'!" );
                else
                {
                    for ( unsigned i = 2; i < tokens.getLength(); i++ )
                        script = script.replaceAll( ( String )"$" + ( i - 2 ), tokens[i] );

                    dbGlobal->executeCommand( script );
                }
            }
            else if ( tokens[0] == "kickall" )
                worldGlobal->removeAllPlayers();
            else if ( tokens[0] == "ls" )
            {
                unsigned interval = 5000;

                if ( !tokens[1].isEmpty() )
                    interval = tokens[1];

                console.writeLine( ( String )" -- " + ( interval / 1000.0f ) + " s to sync" );

                playerList.clear();
                worldGlobal->beginSync( syncPlayer );

                pauseThread( interval );

                worldGlobal->endSync();
                iterate ( playerList )
                    console.writeLine( playerList.current() );
            }
            else if ( tokens[0] == "query" )
                dbGlobal->executeCommand( tokens[1] );
            else if ( tokens[0] == "restart" )
            {
                restart = true;
                break;
            }
            else if ( tokens[0] == "set" )
                dbGlobal->setConfigOption( tokens[1], tokens[2] );
            else if ( tokens[0] == "svmsg" )
                worldGlobal->serverMessage( tokens[1] );
            else if ( tokens[0] == "up" )
            {
                loginGlobal->setStatus( false );
                console.writeLine( " -- Server is back up!" );
            }
            else if ( tokens[0] == "objspawn" )
                worldGlobal->spawnWorldObj( tokens[1], tokens[2], tokens[3], tokens[4] );
            else
                console.writeLine( " -- COMMAND UNRECOGNIZED: " + tokens[0] );
        }

        printf( "## TERM: waiting for all threads to stop...\n" );

        if ( status )
            status->end();

        worldGlobal->end();
        loginGlobal->end();
        pauseThread( 500 );
        printf( "## TERM: killing all remaining threads...\n" );

        printf( "debug: Deleting `dbGlobal`...\n" );
        delete dbGlobal;

        printf( "debug: Deleting `confGlobal`...\n" );
        delete confGlobal;

        printf( "Server shutdown successful.\n\n\n" );
        pauseThread( 500 );
    }
    catch ( Exception ex )
    {
        ex.print();
    }
}

int main( int argc, char** argv )
{
#if defined( _DEBUG ) && defined( _MSC_VER )
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtSetBreakAlloc( 428 );
    printf( "-- DEBUG BUILD --\n" );
#endif

    printf( "\nGlacius - Tales of Lanthaia Game Server\n  copyright (c) 2010, 2011 The Tales of Lanthaia Project\n\n\n" );

    restart = true;

    while ( restart )
    {
        restart = false;
        run( argc > 1 ? argv[1] : "glacius.cfx2" );
    }

    return 0;
}
