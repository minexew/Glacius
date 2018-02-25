
#include <littl.hpp>

namespace Glacius
{
    using namespace li;

    inline String getLocationName( const String& area, const String& zone )
    {
        if ( !zone.isEmpty() )
            return area + " - " + zone;
        else
            return area;
    }
}
