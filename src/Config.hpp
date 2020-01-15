
#include <littl.hpp>
#include <littl/cfx2.hpp>

#include <string>

namespace Glacius
{
    class Config
    {
        std::string fileName;
        cfx2_Node* configDoc;

        public:
            Config( const char* fileName );
            ~Config();

            void commit();

            //cfx2::Node& getNode( const char* path );
            li::String getOption( const char* path, bool optional = false );
            void setOption( const char*, const char* value );
    };
}
