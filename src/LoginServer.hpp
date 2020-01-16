#pragma once

#include <littl.hpp>

#include "PubSub.hpp"

namespace Glacius
{
    using namespace li;

    class Config;
    class Database;
    class LoginServer;

    extern LoginServer* loginGlobal;

    class LoginServer : public Thread, Mutex
    {
        TcpSocket* listener;
        int nextId;

        bool down;
        String reason;

        PubSub::Broker& broker;
        Database& db;

        public:
            LoginServer(PubSub::Broker& broker, Config& config, Database& db);
            virtual ~LoginServer();

            bool isDown( String& why );
            void setStatus( bool maintenance, String why = String() );
            virtual void run();
    };

    class LoginServerSession : public Thread
    {
        TcpSocket* session;
        int id, accountID, charIDs[5];

        PubSub::Broker& broker;
        Database& db;

        public:
            LoginServerSession( TcpSocket* conn, int id, PubSub::Broker& broker, Database& db );
            virtual ~LoginServerSession();

            virtual void run();

            void sendCharacterInfo();
    };
}
