
#include "Config.hpp"
#include "LoginServer.hpp"
#include "ServerState.hpp"
#include "Util.hpp"
#include "WorldServer.hpp"

#include <littl/PerfTiming.hpp>

namespace Glacius
{
    static const uint32_t currentVersion = 3;

    LoginServer* loginGlobal = 0;

    LoginServer::LoginServer(PubSub::Broker& broker, Config& config, Database& db) : nextId( 1 ), down( false ), db(db), sub(broker, myPipe)
    {
        listener = TcpSocket::create( false );

        if ( !listener || !listener->listen( config.getOption( "LoginServer/port" ).toInt() ) )
            throw Exception( "Glacius.LoginServer.StartupFailure", "Login Server startup failed (port already in use?)" );

        sub.add<ServerStateChange>();
    }

    LoginServer::~LoginServer()
    {
        printf( "[T_LS] Login Server shutdown: ok\n" );
        delete listener;
    }

    bool LoginServer::isDown( String& why )
    {
        if ( !down )
            return false;
        else
        {
            enter();
            why = reason;
            leave();

            return true;
        }
    }

    void LoginServer::setStatus( bool maintenance, String why )
    {
        enter();

        down = maintenance;
        reason = why;

        leave();
    }

    void LoginServer::run()
    {
        try
        {
            while ( !shouldEnd )
            {
                while ( auto msg = myPipe.poll() ) {
                    if ( auto req = msg->cast<ServerStateChange>() ) {
                        setStatus(req->state == ServerState::down, req->reason.c_str());
                    }
                }

                TcpSocket* incoming = listener->accept( false );

                if ( incoming )
                {
                    enter();

                    if ( down )
                    {
                        ArrayIOStream buffer;

                        buffer.writeString( "login.server_down" );
                        buffer.writeString( reason );

                        incoming->send( buffer );
                        pauseThread( 3 );
                        incoming->release();
                    }
                    else
                    {
                        LoginServerSession* session = new LoginServerSession( incoming, nextId++, sub.getBroker(), db );
                        session->start();
                    }

                    leave();
                }
                else
                    pauseThread( 200 );
            }
        }
        catch ( Exception ex )
        {
            printf( "[T_LS] Exception:\n" );
            ex.print();
        }
    }

    LoginServerSession::LoginServerSession( TcpSocket* conn, int id, PubSub::Broker& broker, Database& db )
        : session( conn ), id( id ), accountID( -1 ), broker(broker), db(db)
    {
        printf( "[T_LS_%i] Construction (remote IP: %s)\n", id, session->getPeerIP() );
        destroyOnExit();

        session->setBlocking( true );

        for ( int i = 0; i < 5; i++ )
            charIDs[i] = -1;
    }

    LoginServerSession::~LoginServerSession()
    {
        printf( "[T_LS_%i] Destruction.\n", id );

        if ( session )
            delete session;
    }

    void LoginServerSession::run()
    {
        try
        {
            while ( true )
            {
                ArrayIOStream buffer;

                if ( !session->receive( buffer ) )
                    break;

                String message = buffer.readString();

                if ( message == "login.client_hello" )
                {
                    unsigned clientVersion = buffer.read<uint32_t>();

                    printf( "[T_LS_%i] Hello from client. Version %i.\n", id, clientVersion );
                    buffer.clear();

                    if ( clientVersion != currentVersion )
                    {
                        buffer.writeString( "result.err_client_version" );
                        session->send( buffer );
                        break;
                    }

                    buffer.writeString( "login.server_info" );
                    buffer.writeString( db.getConfigOption( "RealmName" ) );
                    buffer.writeString( db.getConfigOption( "RealmMotd" ) );

                    session->send( buffer );
                }
                else if ( message == "login.login_request" )
                {
                    DbOpResult result;

                    String username = buffer.readString();
                    String password = buffer.readString();

                    printf( "[T_LS_%i] Login request. Username %s.\n", id, username.c_str() );
                    buffer.clear();

                    accountID = db.login( username, password, result );

                    if ( accountID >= 0 )
                        buffer.writeString( "result.ok" );
                    else
                        buffer.writeString( result.errorName );

                    session->send( buffer );

                    if ( accountID >= 0 )
                    {
                        printf( "[T_LS_%i] Login successful. Account ID %i.\n", id, accountID );
                        sendCharacterInfo();
                    }
                }
                else if ( message == "login.registration_request" )
                {
                    DbOpResult result;

                    String username = buffer.readString();
                    String password = buffer.readString();

                    printf( "[T_LS_%i] Registration request. Username %s.\n", id, username.c_str() );
                    buffer.clear();

                    if ( username.isEmpty() )
                    {
                        buffer.writeString( "result.err_name_invalid" );
                        session->send( buffer );

                        continue;
                    }

                    accountID = db.createAccount( username, password, result );

                    if ( accountID >= 0 )
                        buffer.writeString( "result.ok" );
                    else
                        buffer.writeString( result.errorName );

                    session->send( buffer );

                    if ( accountID >= 0 )
                    {
                        printf( "[T_LS_%i] Registration successful. Account ID %i.\n", id, accountID );
                        sendCharacterInfo();
                    }
                }
                else if ( message == "login.create_character" )
                {
                    DbOpResult result;

                    String name = buffer.readString();
                    int race = buffer.read<uint16_t>();

                    printf( "[T_LS_%i] Character creation request. Name %s.\n", id, name.c_str() );
                    buffer.clear();

                    // validate session

                    if ( accountID < 0 )
                    {
                        buffer.writeString( "result.err_login_required" );
                        session->send( buffer );
                        continue;
                    }

                    // validate character name

                    bool lastWasAlnum = false;
                    bool isValid = true;

                    for ( StringIter iter( name ); iter; iter++ )
                    {
                        if ( iter == ' ' )
                        {
                            if ( !lastWasAlnum )
                            {
                                isValid = false;
                                break;
                            }

                            lastWasAlnum = false;
                        }
                        else if ( !Unicode::isAlphaNumeric( iter ) )
                        {
                            isValid = false;
                            break;
                        }
                        else
                            lastWasAlnum = true;
                    }

                    if ( !isValid || !lastWasAlnum )
                    {
                        buffer.writeString( "result.err_name_invalid" );
                        session->send( buffer );
                        continue;
                    }

                    // create the characters

                    CharacterCreationInfo creation = { name, race };
                    int charID = db.createCharacter( accountID, creation, result );

                    // send response

                    if ( charID >= 0 )
                        buffer.writeString( "result.ok" );
                    else
                        buffer.writeString( result.errorName );

                    session->send( buffer );

                    if ( charID >= 0 )
                    {
                        printf( "[T_LS_%i] Creation successful. Account ID %i.\n", id, accountID );
                        sendCharacterInfo();
                    }
                }
                else if ( message == "login.enter_world" )
                {
                    int charIndex = buffer.read<uint16_t>();

                    buffer.clear();

                    if ( accountID < 0 )
                    {
                        buffer.writeString( "result.err_login_required" );
                        session->send( buffer );
                        continue;
                    }

                    if ( charIndex < 0 || charIndex >= 5 || charIDs[charIndex] < 0 )
                    {
                        buffer.writeString( "result.err_invalid_char" );
                        session->send( buffer );
                        continue;
                    }

                    buffer.writeString( "login.entering_world" );
                    session->send( buffer );

                    printf( "[T_LS_%i] Entering world with character #%i!!\n", id, charIDs[charIndex] );
                    broker.publish<WorldEnterRequest>(unique_ptr<TcpSocket>(session), charIDs[charIndex]);

                    session = 0;
                    break;
                }
                else if ( message != "login.keep_alive" )
                {
                    printf( "[T_LS_%i] Epic fail. Unknown message '%s'\n", id, message.c_str() );
                    break;
                }
            }
        }
        catch ( Exception ex )
        {
            printf( "[T_LS_%i] Exception:\n", id );
            ex.print();
        }
    }

    void LoginServerSession::sendCharacterInfo()
    {
        ArrayIOStream buffer;
        buffer.writeString( "login.character_info" );

        CharacterSummary chars[5];
        int count = db.getCharacterList( accountID, chars );

        buffer.write<uint16_t>( count );

        for ( int i = 0; i < count; i++ )
        {
            printf( "name %s, area %s, zone %s, id %i, race %i, class %i, level %i\n",
                    chars[i].name.c_str(), chars[i].area.c_str(), chars[i].zone.c_str(), chars[i].charID, chars[i].race, chars[i].classID, chars[i].level );

            buffer.writeString( chars[i].name );
            buffer.writeString( getLocationName( chars[i].area, chars[i].zone ) );
            buffer.write<uint16_t>( chars[i].race );
            buffer.write<uint16_t>( chars[i].classID );
            buffer.write<uint16_t>( chars[i].level );

            charIDs[i] = chars[i].charID;
        }

        session->send( buffer );
    }
}
