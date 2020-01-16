
#include "Config.hpp"
#include "StatusServer.hpp"

namespace Glacius
{
    StatusServer::StatusServer(PubSub::Broker& broker, Config& config, Database& db) : db(db), sub(broker, myPipe)
    {
        int port = config.getOption( "StatusServer/port" ).toInt();
        listener = TcpSocket::create( false );

        if ( !listener || !listener->listen( port ) )
            throw Exception( "Glacius.StatusServer.StartFailure", ( String )"Status Server startup at port " + port + " failed" );

        sub.add<ServerStateChange>();
        sub.add<WorldStatusAnnouncement>();
        broker.publish<Request<WorldStatusAnnouncement>>();
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
                while ( auto msg = myPipe.poll() ) {
                    if ( auto stateChange = msg->cast<ServerStateChange>() ) {
                        serverState = *stateChange;
                    }
                    else if ( auto announcement = msg->cast<WorldStatusAnnouncement>() ) {
                        worldStatus = *announcement;
                    }
                }

                TcpSocket* incoming = listener->accept( false );

                if ( incoming )
                {
                    StatusServerSession* session = new StatusServerSession( incoming, sub.getBroker(), db, *this );
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

    StatusServerSession::StatusServerSession( TcpSocket* conn, PubSub::Broker& broker, Database& db, StatusServer& status )
            : session( conn ), broker(broker), db(db), status_(status)
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

        std::promise<std::vector<std::string>> promise;
        auto future = promise.get_future();

        broker.publish<CharacterListQuery>(std::move(promise));
        auto list = future.get();

        String payload;

        for ( auto characterName : list ) {
            payload += ( String ) "Player: " + characterName.c_str() + "\n";
        }

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
                session->write( "Location: " + db.getConfigOption( "RealmWebsite" ) + "\r\n" );
                session->write( "\r\n" );
            }
            else
                break;
        }
    }

    void StatusServerSession::status()
    {
        String payload = "RealmName: " + db.getConfigOption( "RealmName" ) + "\n";

        auto serverState = status_.getServerState();

        if ( serverState.state == ServerState::down )
        {
            payload += "Status: down\n";
            payload += ( String ) "Reason: " + serverState.reason.c_str() + "\n";
        }
        else
        {
            payload += "Status: online\n";
            payload += ( String ) "Population: " + status_.getNumPlayersOnline();
        }

        session->write( "HTTP/1.1 200 OK\r\n" );
        session->write( ( String ) "Content-Length: " + payload.getNumBytes() + "\r\n" );
        session->write( "\r\n" );
        session->write( payload );
    }
}

