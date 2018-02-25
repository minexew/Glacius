
#include <littl.hpp>
#include <littl/cfx2.hpp>

#include "Database.hpp"

namespace Glacius
{
    using namespace li;

    class Config;

    extern Config* confGlobal;

    class Config
    {
        String fileName;
        cfx2_Node* configDoc;

        public:
            Config( const String& fileName );
            ~Config();

            //cfx2::Node& getNode( const char* path );
            String getOption( const char* path, bool optional = false );
            void setOption( const char* path, const char* value );
    };
}
