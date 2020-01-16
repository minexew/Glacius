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
    struct WorldStatusAnnouncement;

    class StatusServer : public Thread
    {
        TcpSocket* listener;
        Database& db;

        public:
            StatusServer(PubSub::Broker& broker, Config& config, Database& db);
            virtual ~StatusServer();

            int getNumPlayersOnline() { return worldStatus.playersOnline; }
            ServerStateChange getServerState() { return serverState; } // FIXME: this is NOT thread-safe!

            virtual void run();

    private:
        PubSub::Pipe myPipe;
        PubSub::Subscription sub;

        // these are mostly incorrect (should start as nullopt, OR be required in constructor)
        ServerStateChange serverState {};
        WorldStatusAnnouncement worldStatus {};
    };

    class StatusServerSession : public Thread
    {
        TcpSocket* session;
        Database& db;

        public:
            StatusServerSession( TcpSocket* conn, PubSub::Broker& broker, Database& db, StatusServer& status );
            virtual ~StatusServerSession();

            void listPlayers();
            virtual void run();
            void status();

    private:
        PubSub::Broker& broker;
        StatusServer& status_;
    };
}
