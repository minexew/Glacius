#pragma once

#include <littl.hpp>

namespace Glacius
{
    using namespace li;

    class Config;
    class LoginServer;

    extern LoginServer* loginGlobal;

    class LoginServer : public Thread, Mutex
    {
        TcpSocket* listener;
        int nextId;

        bool down;
        String reason;

        public:
            LoginServer(Config& config);
            virtual ~LoginServer();

            bool isDown( String& why );
            void setStatus( bool maintenance, String why = String() );
            virtual void run();
    };

    class LoginServerSession : public Thread
    {
        TcpSocket* session;
        int id, accountID, charIDs[5];

        public:
            LoginServerSession( TcpSocket* conn, int id );
            virtual ~LoginServerSession();

            virtual void run();

            void sendCharacterInfo();
    };
}
