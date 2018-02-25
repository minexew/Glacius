#pragma once

#include "Database.hpp"

#include <littl.hpp>

namespace Glacius
{
    class PrecTimer
    {
        LARGE_INTEGER begin, freq;

        public:
            PrecTimer()
            {
                QueryPerformanceFrequency( &freq );
            }

            void start()
            {
                QueryPerformanceCounter( &begin );
            }

            __int64 stop()
            {
                LARGE_INTEGER end;

                QueryPerformanceCounter( &end );
                return ( end.QuadPart - begin.QuadPart ) * 1000000 / freq.QuadPart;
            }
    };
}

namespace Glacius
{
    using namespace li;

    static const unsigned invalidPid = 0xFFFFFFFF;

    class WorldServer;
    class WorldServerSession;

    extern WorldServer* worldGlobal;

    typedef void ( *SyncCallback )( unsigned pid, const String& name, unsigned interval, float x, float y );

    class WorldServer : public Thread, public Mutex
    {
        List<WorldServerSession*> clients;
        unsigned numOnline;

        clock_t syncBegin;
        SyncCallback onSync;

        void broadcast( const StreamBuffer<>& buffer, unsigned excludePid );
        unsigned registerSession( WorldServerSession* session, CharacterProperties& props );
        void unregisterSession( unsigned pid, CharacterProperties& props );
        static void writePlayerListItem( StreamBuffer<>& buffer, unsigned pid, CharacterProperties& props );

        friend class WorldServerSession;

        public:
            WorldServer();
            virtual ~WorldServer();

            void beginSync( SyncCallback callback );
            void broadcast( CharacterProperties& broadcaster, const String& message );
            void endSync();
            unsigned getNumOnline() const { return numOnline; }
            void getPlayersOnline( List<String>& players );
            void playerMoved( unsigned pid, CharacterProperties& broadcaster );
            void removeAllPlayers();
            void removeWorldObj( float x, float y );
            virtual void run();
            void serverMessage( const String& message );
            void spawnWorldObj( const String& name, float x, float y, float o );
            void sync( unsigned pid, CharacterProperties& props );
    };

    class WorldServerSession : public Thread, public Mutex
    {
        TcpSocket* session;
        unsigned pid;

        CharacterProperties props;

        void sendWelcome();

        friend class WorldServer;

        public:
            WorldServerSession( TcpSocket* conn, int charID );
            virtual ~WorldServerSession();

            const String& getName() const;
            void send( const StreamBuffer<>& buffer );
            virtual void run();
            void writePlayerListItem( StreamBuffer<>& buffer );
    };
}
