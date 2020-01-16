#pragma once

#include "PubSub.hpp"
#include "WorldServer.hpp"
#include "ServerState.hpp"

#include <littl.hpp>

#include <vector>

namespace Glacius
{
    using namespace li;

    class Config;
    class Database;
    class LoginServer;
    struct WorldStatusAnnouncement;

    class StatusServer : public Thread
    {
        TcpSocket* listener;
        Database& db;
        LoginServer& login;

        public:
            StatusServer(PubSub::Broker& broker, Config& config, Database& db, LoginServer& login);
            virtual ~StatusServer();

            int getNumPlayersOnline() { return worldStatus.playersOnline; }

            virtual void run();

    private:
        PubSub::Pipe myPipe;
        PubSub::Subscription sub;

        // these are mostly incorrect (should start as nullopt)
        ServerStateChange serverState {};
        WorldStatusAnnouncement worldStatus {};
    };

    class StatusServerSession : public Thread
    {
        TcpSocket* session;
        Database& db;
        LoginServer& login;

        public:
            StatusServerSession( TcpSocket* conn, PubSub::Broker& broker, Database& db, LoginServer& login, StatusServer& status );
            virtual ~StatusServerSession();

            void listPlayers();
            virtual void run();
            void status();

    private:
        PubSub::Broker& broker;
        StatusServer& status_;
    };
}
