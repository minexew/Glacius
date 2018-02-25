
#include <littl>

using namespace li;

static TcpSocket* sock;

bool receive()
{
    StreamBuffer<> buffer;

    if ( sock->receive( buffer ) )
    {
        String message = buffer.readString();

        if ( message == "result.ok" )
            printf( "Ok.\n" );
        else if ( message == "result.err_account_exists" )
            printf( "Error: that account already exists!\n" );
        else if ( message == "result.err_character_exists" )
            printf( "Error: that character already exists!\n" );
        else if ( message == "result.err_client_version" )
            printf( "Error: your client version is just wrong!\n" );
        else if ( message == "result.err_login_incorrect" )
            printf( "Error: too bad, incorrect username and/or password!\n" );
        else if ( message == "login.server_info" )
        {
            String serverName = buffer.readString();
            String serverNews = buffer.readString();
            printf( "Server: %s\n\tnews: %s\n", serverName.c_str(), serverNews.c_str() );
        }
        else if ( message == "login.character_info" )
        {
            int count = buffer.read<short>();

            for ( int i = 0; i < count; i++ )
            {
                printf( "Character: %s\n", buffer.readString().c_str() );
                printf( "\tzone: %s\n", buffer.readString().c_str() );
                printf( "\trace: %i\n", buffer.read<short>() );
                printf( "\tclass: %i\n", buffer.read<short>() );
                printf( "\tlevel: %i\n", buffer.read<short>() );
            }
        }
        else if ( message == "login.entering_world" )
            printf( "Welcome to the world!\n" );
        else
            printf( "Unknown message '%s'\n", message.c_str() );

        return true;
    }
    else
        return false;
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        printf( "usage: GlaciusRCC <host>\n" );
        return 0;
    }

    sock = new TcpSocket( false );

    if ( !sock->connect( argv[1], 0x6141 ) )
    {
        printf( "unable to connect to '%s'\n", argv[1] );
        return 0;
    }

    while ( sock->isReadable() )
    {
        StreamBuffer<> buffer;
        Console console;

        printf( "\n> " );
        String command = console.readLine();

        if ( command == "hello" )
        {
            buffer.writeString( "login.client_hello" );
            buffer.write<short>( 1 );
        }
        else if ( command == "login" )
        {
            buffer.writeString( "login.login_request" );

            printf( "username: " );
            buffer.writeString( console.readLine() );

            printf( "password: " );
            buffer.writeString( console.readLine() );
        }
        else if ( command == "register" )
        {
            buffer.writeString( "login.registration_request" );

            printf( "username: " );
            buffer.writeString( console.readLine() );

            printf( "password: " );
            buffer.writeString( console.readLine() );
        }
        else if ( command == "newchar" )
        {
            buffer.writeString( "login.create_character" );

            printf( "name: " );
            buffer.writeString( console.readLine() );

            printf( "race: " );
            buffer.write<short>( console.readLine() );
        }
        else if ( command == "play" )
        {
            buffer.writeString( "login.enter_world" );

            printf( "character (0-4): " );
            buffer.write<short>( console.readLine() );
        }
        else
        {
            receive();
            continue;
        }

        sock->send( buffer );

        unsigned start = GetTickCount();

        while ( sock->isReadable() && !receive() )
        {
            if ( GetTickCount() > start + 7000 )
            {
                printf( "[Warning] Response timed out.\n" );
                break;
            }

            Sleep( 200 );
        }
    }

    delete sock;
    return 0;
}
