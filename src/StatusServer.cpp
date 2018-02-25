
#include "Config.hpp"
#include "StatusServer.hpp"
#include "WorldServer.hpp"

namespace Glacius
{
    StatusServer::StatusServer()
    {
        int port = confGlobal->getOption( "StatusServer/port" );
        listener = new TcpSocket( false );

        if ( !listener || !listener->listen( port ) )
            throw Exception( "Glacius.StatusServer.StartFailure", ( String )"Status Server startup at port " + port + " failed" );

        destroyOnExit();
    }

    StatusServer::~StatusServer()
    {
        Console::write( "[Status Server] Shutting down Status Server..." );

        delete listener;

        Console::writeLine( "ok" );
    }

    void StatusServer::run()
    {
        try
        {
            while ( !shouldEnd )
            {
                TcpSocket* incoming = listener->accept();

                if ( incoming )
                {
                    StatusServerSession* session = new StatusServerSession( incoming );
                    session->start();
                }
                else
                    pauseThread( 50 );
            }
        }
        catch ( Exception ex )
        {
            printf( "[Status Server] Exception Caught:\n" );
            ex.print();
        }
    }

    StatusServerSession::StatusServerSession( TcpSocket* conn ) : session( conn )
    {
        session->setBlocking( true );
        destroyOnExit();
    }

    StatusServerSession::~StatusServerSession()
    {
        pauseThread( 5000 );
        session->release();
    }

    void StatusServerSession::listPlayers()
    {
        List<String> players;

        worldGlobal->getPlayersOnline( players );

        String payload;

        iterate ( players )
            payload += "Player: " + players.current() + "\n";

        session->write( "HTTP/1.1 200 OK\r\n" );
        session->write( ( String )"Content-Length: " + payload.getNumBytes() + "\r\n" );
        session->write( "\r\n" );
        session->write( payload );
    }

    void StatusServerSession::run()
    {
        while ( session->isWritable() )
        {
            // parse a single request

            String uri;

            while ( session->isWritable() )
            {
                String line = session->readLine();

                if ( line.isEmpty() )
                    break;

                if ( line.beginsWith( "GET " ) )
                {
                    int pageBegin = line.findChar( ' ' );
                    int pageEnd = line.findChar( ' ', pageBegin + 1 );

                    uri = line.leftPart( pageEnd ).dropLeftPart( pageBegin + 1 );
                }
            }

            if ( uri == "/players" )
                listPlayers();
            else if ( uri == "/status" )
                status();
            else if ( !uri.isEmpty() )
            {
                session->write( "HTTP/1.1 301 Moved Permanently\r\n" );
                session->write( "Content-Length: 0\r\n" );
                session->write( "Location: " + dbGlobal->getConfigOption( "RealmWebsite" ) + "\r\n" );
                session->write( "\r\n" );
            }
            else
                break;
        }
    }

    void StatusServerSession::status()
    {
        String reason;

        String payload = "RealmName: " + dbGlobal->getConfigOption( "RealmName" ) + "\n";

        if ( loginGlobal->isDown( reason ) )
        {
            payload += "Status: down\n";
            payload += "Reason: " + reason + "\n";
        }
        else
        {
            payload += "Status: online\n";
            payload += ( String ) "Population: " + worldGlobal->getNumOnline();
        }

        session->write( "HTTP/1.1 200 OK\r\n" );
        session->write( ( String ) "Content-Length: " + payload.getNumBytes() + "\r\n" );
        session->write( "\r\n" );
        session->write( payload );
    }
}

