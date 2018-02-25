#pragma once

#include <littl.hpp>

#include "LoginServer.hpp"

namespace Glacius
{
    using namespace li;

    class StatusServer : public Thread
    {
        TcpSocket* listener;

        public:
            StatusServer();
            virtual ~StatusServer();

            virtual void run();
    };

    class StatusServerSession : public Thread
    {
        TcpSocket* session;

        public:
            StatusServerSession( TcpSocket* conn );
            virtual ~StatusServerSession();

            void listPlayers();
            virtual void run();
            void status();
    };
}
