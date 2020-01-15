
#include "Config.hpp"
#include "Database.hpp"

namespace Glacius
{
    static const char* defaultsFileName = "glacius.default.cfx2";

    Config::Config( const char* fileName ) : fileName( fileName )
    {
        configDoc = cfx2_load_document( fileName );

        if ( !configDoc )
        {
            Console::writeLine( "[Config] NOTE: Configuration file not found, loading defaults" );
            configDoc = cfx2_load_document( defaultsFileName );
        }

        if ( !configDoc )
            throw Exception( "Glacius.Config.FileNotFound", "Can't load configuration file " + File::formatFileName( fileName )
                    + " nor defaults file " + File::formatFileName( defaultsFileName ) );
    }

    Config::~Config()
    {
        cfx2_release_node( configDoc );
    }

    void Config::commit()
    {
        cfx2_save_document( configDoc, fileName.c_str() );
    }

    /*cfx2::Node& Config::getNode( const char* path )
    {
        return cfx2_query_node( configDoc, path, 0 );
    }*/

    String Config::getOption( const char* path, bool optional )
    {
        const char* option = cfx2_query_value( configDoc, path );

        if ( !option && !optional )
            throw Exception( "Glacius.Config.MissingOption", ( String ) "Missing requested option " + path );

        return option;
    }

    void Config::setOption( const char* path, const char* value )
    {
        cfx2_query( configDoc, ( String ) path + ":" + value, 1, 0 );
    }
}
