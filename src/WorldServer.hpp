#pragma once

#include "Database.hpp"
#include "PubSub.hpp"

#include <littl.hpp>

#include <future>
#include <vector>

namespace Glacius
{
    using namespace li;

    static const unsigned invalidPid = 0xFFFFFFFF;

    class WorldServer;
    class WorldServerSession;

    typedef void ( *SyncCallback )( unsigned pid, const String& name, unsigned interval, float x, float y );

    template <typename T>
    struct Request {};

    struct CharacterListQuery
    {
        std::promise<std::vector<std::string>> promise;
    };

    struct WorldEnterRequest
    {
        unique_ptr<TcpSocket> socket;
        int charID;
    };

    struct WorldStatusAnnouncement
    {
        int playersOnline;
    };

    class WorldServer : public Thread, public Mutex
    {
        List<WorldServerSession*> clients;
        unsigned numOnline;

        clock_t syncBegin;
        SyncCallback onSync;

        Database& db;
        PubSub::Pipe myPipe;
        PubSub::Subscription sub;

        void broadcast( const ArrayIOStream& buffer, unsigned excludePid );
        unsigned registerSession( WorldServerSession* session, CharacterProperties& props );
        void unregisterSession( unsigned pid, CharacterProperties& props );
        static void writePlayerListItem( ArrayIOStream& buffer, unsigned pid, CharacterProperties& props );

        friend class WorldServerSession;

        public:
            WorldServer(PubSub::Broker& broker, Database& db);
            virtual ~WorldServer();

            void beginSync( SyncCallback callback );
            void broadcast( CharacterProperties& broadcaster, const String& message );
            void endSync();
            unsigned getNumOnline() const { return numOnline; }
            void playerMoved( unsigned pid, CharacterProperties& broadcaster );
            void removeAllPlayers();
            void removeWorldObj( float x, float y );
            virtual void run();
            void serverMessage( const String& message );
            void spawnWorldObj( const String& name, float x, float y, float o );
            void sync( unsigned pid, CharacterProperties& props );

    private:
        std::vector<std::string> getPlayersOnline();
    };

    class WorldServerSession : public Thread, public Mutex
    {
        TcpSocket* session;
        unsigned pid;

        Database& db;

        CharacterProperties props;

        void sendWelcome();

        friend class WorldServer;

        public:
            WorldServerSession( TcpSocket* conn, int charID, Database& db, WorldServer& world );
            virtual ~WorldServerSession();

            const String& getName() const;
            void send( const ArrayIOStream& buffer );
            virtual void run();
            void writePlayerListItem( ArrayIOStream& buffer );

    private:
        WorldServer& world;
    };
}
