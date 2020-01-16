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
        LoginServer& login;

        public:
            StatusServer(Config& config, Database& db, LoginServer& login);
            virtual ~StatusServer();

            virtual void run();
    };

    class StatusServerSession : public Thread
    {
        TcpSocket* session;
        Database& db;
        LoginServer& login;

        public:
            StatusServerSession( TcpSocket* conn, Database& db, LoginServer& login );
            virtual ~StatusServerSession();

            void listPlayers();
            virtual void run();
            void status();
    };
}
