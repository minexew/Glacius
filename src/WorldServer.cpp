
#include "Config.hpp"
#include "Util.hpp"
#include "WorldServer.hpp"

#include <Messages.hpp>

#include <littl/PerfTiming.hpp>

using namespace GameClient;

namespace Glacius
{
    WorldServer::WorldServer(PubSub::Broker& broker, Database& db) : numOnline( 0 ), db(db), sub(broker, myPipe)
    {
        broker.publish<WorldStatusAnnouncement>(0);

        sub.add<Request<WorldStatusAnnouncement>>();
        sub.add<CharacterListQuery>();
        sub.add<WorldEnterRequest>();
    }

    WorldServer::~WorldServer()
    {
        printf( "[T_WS] World Server shutdown: ok\n" );
    }

    void WorldServer::broadcast( const ArrayIOStream& buffer, unsigned excludePid )
    {
        PerfTimer tm;
        auto start = tm.getCurrentMicros();

        for ( unsigned i = 0; i < clients.getLength(); i++ )
        {
            WorldServerSession* client = clients[i];

            if ( client && client->pid != excludePid )
                client->send( buffer );
        }

        unsigned bcastTime = tm.getCurrentMicros() - start;

        if ( bcastTime > 100 )
            printf( "WARNING: WorldServer::broadcast [%u us]\n", bcastTime );
    }

    void WorldServer::broadcast( CharacterProperties& broadcaster, const String& message )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::chat_message );
        buffer.write<uint16_t>( 0 );//channel
        buffer.writeString( broadcaster.name );
        buffer.writeString( message );

        broadcast( buffer, invalidPid );
    }

    std::vector<CharacterListQuery::Entry> WorldServer::getPlayersOnline()
    {
        std::vector<CharacterListQuery::Entry> players_out;

        for ( unsigned i = 0; i < clients.getLength(); i++ )
        {
            WorldServerSession* client = clients[i];

            if ( client )
                players_out.push_back( {(int) client->pid, client->getName().c_str(), Vec3f {client->props.x, client->props.y, client->props.z}} );
        }

        return players_out;
    }

    void WorldServer::playerMoved( unsigned pid, CharacterProperties& props )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::player_location );
        buffer.write<uint16_t>( pid );
        buffer.write<float>( props.x );
        buffer.write<float>( props.y );
        buffer.write<float>( props.z );
        buffer.write<float>( props.orientation );

        broadcast( buffer, pid );
    }

    unsigned WorldServer::registerSession( WorldServerSession* session, CharacterProperties& props )
    {
        enter();
        unsigned pid = clients.add( session );
        numOnline++;
        leave();

        // Inform others of the player becoming online
        ArrayIOStream notifyOnlineBuf;
        notifyOnlineBuf.write<uint16_t>( world::player_status );
        notifyOnlineBuf.write<uint16_t>( pid );
        notifyOnlineBuf.writeString( props.name );
        notifyOnlineBuf.write<uint16_t>( status::online );

        // Add the player to those near him
        ArrayIOStream addPlayerBuf;
        addPlayerBuf.write<uint16_t>( world::player_list );
        addPlayerBuf.write<uint16_t>( 1 );
        writePlayerListItem( addPlayerBuf, pid, props );

        // Retrieve the list of other players
        ArrayIOStream listPlayersBuf;
        listPlayersBuf.write<uint16_t>( world::player_list );

        listPlayersBuf.write<uint16_t>( numOnline - 1 );

        for ( unsigned i = 0; i < clients.getLength(); i++ )
        {
            WorldServerSession* client = clients[i];

            if ( i != pid && client )
            {
                client->send( notifyOnlineBuf );

                // TODO: check if in area
                client->send( addPlayerBuf );
                client->writePlayerListItem( listPlayersBuf );
            }
        }

        session->send( listPlayersBuf );

        // Announce onto message bus
        sub.getBroker().publish<WorldStatusAnnouncement>((int) clients.getLength());

        return pid;
    }

    void WorldServer::removeAllPlayers()
    {
        iterate ( clients )
            if ( clients.current() )
                clients.current()->end();
    }

    void WorldServer::removeWorldObj( float x, float y )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::remove_world_obj );
        buffer.write<float>( x );
        buffer.write<float>( y );

        broadcast( buffer, invalidPid );
    }

    void WorldServer::run()
    {
        try
        {
            while ( !shouldEnd )
            {
                /*
                enter();
                ...
                leave();
                */

                while ( auto msg = myPipe.poll() ) {
                    if ( auto req = msg->cast<CharacterListQuery>() ) {
                        req->promise.set_value(this->getPlayersOnline());
                    }
                    if ( auto req = msg->cast<Request<WorldStatusAnnouncement>>() ) {
                        sub.getBroker().publish<WorldStatusAnnouncement>((int) clients.getLength());
                    }
                    else if ( auto req = msg->cast<WorldEnterRequest>() ) {
                        WorldServerSession* wss = new WorldServerSession(req->socket.release(), req->charID, db, *this);
                        wss->start();
                    }
                }

                pauseThread( 50 );
            }

            iterate ( clients )
                if ( clients.current() )
                    clients.current()->end();
        }
        catch ( Exception ex )
        {
            printf( "[T_WS] Exception:\n" );
            ex.print();
        }
    }

    void WorldServer::serverMessage( const String& message )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::server_message );
        buffer.writeString( message );

        broadcast( buffer, invalidPid );
    }

    void WorldServer::spawnWorldObj( const String& name, float x, float y, float o )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::spawn_world_obj );
        buffer.writeString( name );
        buffer.write<float>( x );
        buffer.write<float>( y );
        buffer.write<float>( o );

        broadcast( buffer, invalidPid );
    }

    void WorldServer::unregisterSession( unsigned pid, CharacterProperties& props )
    {
        ArrayIOStream buffer;
        buffer.write<uint16_t>( world::player_status );
        buffer.write<uint16_t>( pid );
        buffer.writeString( props.name );
        buffer.write<uint16_t>( status::offline );

        broadcast( buffer, pid );

        clients[pid] = 0;
        numOnline--;
    }

    void WorldServer::writePlayerListItem( ArrayIOStream& buffer, unsigned pid, CharacterProperties& props )
    {
        buffer.write<uint16_t>( pid );
        buffer.writeString( props.name );
        buffer.write<uint16_t>( props.race );
        buffer.write<uint16_t>( props.classID );
        buffer.write<uint16_t>( props.level );
        buffer.write<float>( props.x );
        buffer.write<float>( props.y );
        buffer.write<float>( props.z );
        buffer.write<float>( props.orientation );
    }

    WorldServerSession::WorldServerSession( TcpSocket* conn, int charID, Database& db, WorldServer& world )
        : session( conn ), db( db ), world(world)
    {
        db.loadCharacter( charID, &props );
        pid = world.registerSession( this, props );

        printf( "[T_WS_%i] Entering world.\n", pid );
        destroyOnExit();

        session->setBlocking( false );
    }

    WorldServerSession::~WorldServerSession()
    {
        printf( "[T_WS_%i] Destruction.\n", pid );
        delete session;
    }

    const String& WorldServerSession::getName() const
    {
        return props.name;
    }

    void WorldServerSession::send( const ArrayIOStream& buffer )
    {
        //enter();
        session->send( buffer );
        //leave();
    }

    void WorldServerSession::run()
    {
        try
        {
            ArrayIOStream buffer;

            enter();
            sendWelcome();
            leave();

            while ( session->isWritable() && !shouldEnd )
            {
                PerfTimer tm;
                auto start = tm.getCurrentMicros();

                enter();
                bool received = session->receive( buffer );
                leave();

                unsigned bcastTime = tm.getCurrentMicros() - start;

                if ( bcastTime > 400 )
                    printf( "WARNING: Session::receive [%u us]\n", bcastTime );

                if ( received )
                {
                    short message = buffer.read<uint16_t>();

                    switch ( message )
                    {
                        case world::nop:
                            break;

                        case world::player_movement:
                            props.x = buffer.read<float>();
                            props.y = buffer.read<float>();
                            props.z = buffer.read<float>();
                            props.orientation = buffer.read<float>();

                            world.playerMoved( pid, props );
                            break;

                        case world::say:
                        {
                            String message = buffer.readString();
                            printf( "# %s: %s\n", props.name.c_str(), message.c_str() );

                            if ( !message.beginsWith( '/' ) )
                                world.broadcast( props, message );
                            else
                            {
                                List<String> tokens;

                                message.parse( tokens, ' ', '\\' );

                                if ( tokens[0] == "/obj" )
                                {
                                    if ( tokens[1] == "spawn" )
                                    {
                                        if ( !tokens[3].isEmpty() )
                                            world.spawnWorldObj( tokens[2], tokens[3], tokens[4], tokens[5] );
                                        else
                                            world.spawnWorldObj( tokens[2], props.x, props.y, props.orientation );
                                    }
                                    else if ( tokens[1] == "remove" )
                                    {
                                        if ( !tokens[3].isEmpty() )
                                            world.removeWorldObj( tokens[2], tokens[3] );
                                        else
                                            world.removeWorldObj( props.x, props.y );
                                    }
                                }
                            }
                            break;
                        }

                        case world::sync:
                            // TODO: update ping
                            break;

                        default:
                            printf( "[T_WS_%i] Epic fail. Unknown message %04X\n", pid, message );
                    }
                }

                pauseThread( 1 );
            }
        }
        catch ( Exception ex )
        {
            printf( "[T_WS_%i] Exception:\n", pid );
            ex.print();
        }

        world.unregisterSession( pid, props );

        printf( "[T_WS_%i] Saving character %s\n", pid, props.name.c_str() );
        db.saveCharacter( &props );

        // Even after unregistering, some iterating methods may still hold the session pointer
        pauseThread( 100 );
    }

    void WorldServerSession::sendWelcome()
    {
        ArrayIOStream buffer;

        buffer.write<uint16_t>( world::welcome );
        buffer.write<uint16_t>( pid );
        buffer.writeString( props.name );
        buffer.writeString( getLocationName( props.area, props.zone ) );
        buffer.write<uint16_t>( props.race );
        buffer.write<uint16_t>( props.classID );
        buffer.write<uint16_t>( props.level );
        buffer.write<uint16_t>( props.zoneID );
        buffer.write<float>( props.x );
        buffer.write<float>( props.y );
        buffer.write<float>( props.z );
        buffer.write<float>( props.orientation );
        buffer.write<uint32_t>( props.gold );

        session->send( buffer );
    }

    void WorldServerSession::writePlayerListItem( ArrayIOStream& buffer )
    {
        //enter();
        WorldServer::writePlayerListItem( buffer, pid, props );
        //leave();
    }
}
