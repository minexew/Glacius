#pragma once

#include <littl.hpp>

#include "LoginServer.hpp"

namespace Glacius
{
    using namespace li;

    class Config;

    class StatusServer : public Thread
    {
        TcpSocket* listener;
        Database& db;

        public:
            StatusServer(Config& config, Database& db);
            virtual ~StatusServer();

            virtual void run();
    };

    class StatusServerSession : public Thread
    {
        TcpSocket* session;
        Database& db;

        public:
            StatusServerSession( TcpSocket* conn, Database& db );
            virtual ~StatusServerSession();

            void listPlayers();
            virtual void run();
            void status();
    };
}
