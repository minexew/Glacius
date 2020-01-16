
#if defined( _DEBUG ) && defined( _MSC_VER )
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <littl.hpp>

#include "Config.hpp"
#include "IOUtil.hpp"
#include "LoginServer.hpp"
#include "ServerState.hpp"
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

static void run( const char* configFile )
{
    try
    {
        PubSub::Broker broker;

        printf( "## reading configuration file: %s\n", configFile );
        Config config( configFile );

        printf( "## creating database session...\n" );
        auto the_db = Database::create(config);
        auto& db = *the_db;

        printf( "## starting Login Server\n" );
        LoginServer login(broker, config, db);
        login.start();
        pauseThread( 200 );

        printf( "## starting World Server\n" );
        WorldServer world(broker, db);
        world.start();
        pauseThread( 200 );

        StatusServer* status = 0;

        if ( config.getOption( "StatusServer/enabled", true ).toInt() )
        {
            printf( "## starting Status Server\n" );
            status = new StatusServer(broker, config, db);
            status->start();
            pauseThread( 200 );
        }

        if ( config.getOption( "Server/isConfigured", true ).isEmpty() )
        {
            broker.publish<ServerStateChange>( ServerState::down, "Initial configuration." );

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

            config.setOption( "Server/isConfigured", "1" );
        }
        else {
            broker.publish<ServerStateChange>( ServerState::up, "" );
        }

        printf( "\n\n-------------------------------------------------------------------------------\n" );
        printf( "## The server is now running. Type \"help\" for a list of server commands.\n" );
        printf( "## To stop the server, type \"exit\".\n" );
        printf( "-------------------------------------------------------------------------------\n\n" );
        printf( "\n" );

        while ( IOUtil::isStdoutATty() )
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
                broker.publish<ServerStateChange>(ServerState::down, tokens[1].c_str());
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
                        String value = db.getConfigOption( tokens[i] );
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

                    db.executeCommand( script );
                }
            }
            else if ( tokens[0] == "kickall" )
                world.removeAllPlayers();
            else if ( tokens[0] == "ls" )
            {
                unsigned interval = 5000;

                if ( !tokens[1].isEmpty() )
                    interval = tokens[1];

                console.writeLine( ( String )" -- " + ( interval / 1000.0f ) + " s to sync" );

                playerList.clear();
                world.beginSync( syncPlayer );

                pauseThread( interval );

                world.endSync();
                iterate ( playerList )
                    console.writeLine( playerList.current() );
            }
            else if ( tokens[0] == "query" )
                db.executeCommand( tokens[1] );
            else if ( tokens[0] == "restart" )
            {
                restart = true;
                break;
            }
            else if ( tokens[0] == "set" )
                db.setConfigOption( tokens[1], tokens[2] );
            else if ( tokens[0] == "svmsg" )
                world.serverMessage( tokens[1] );
            else if ( tokens[0] == "up" )
            {
                broker.publish<ServerStateChange>(ServerState::up, "");
                console.writeLine( " -- Server is back up!" );
            }
            else if ( tokens[0] == "objspawn" )
                world.spawnWorldObj( tokens[1], tokens[2], tokens[3], tokens[4] );
            else
                console.writeLine( " -- COMMAND UNRECOGNIZED: " + tokens[0] );
        }

        while ( !IOUtil::isStdoutATty() ) {
            IOUtil::waitForSignal();
        }

        printf( "## TERM: waiting for all threads to stop...\n" );

        if ( status )
            status->end();

        world.end();
        login.end();
        pauseThread( 500 );

        world.waitFor();
        login.waitFor();

        printf( "## TERM: killing all remaining threads...\n" );

        // FIXME: this will break all hell if the threads are not stopped
        the_db.reset();
        config.commit();

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
    // disable stdout buffering since we abuse it for stderr
    IOUtil::disableStdoutBuffering();

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
