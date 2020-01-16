#pragma once

#include <littl.hpp>

#include "PubSub.hpp"

namespace Glacius
{
    using namespace li;

    class Config;
    class Database;
    class LoginServer;

    class LoginServer : public Thread, Mutex
    {
        TcpSocket* listener;
        int nextId;

        bool down;
        String reason;

        Database& db;

        public:
            LoginServer(PubSub::Broker& broker, Config& config, Database& db);
            virtual ~LoginServer();

            bool isDown( String& why );
            virtual void run();

    private:
        void setStatus( bool maintenance, String why );

        PubSub::Pipe myPipe;
        PubSub::Subscription sub;
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
