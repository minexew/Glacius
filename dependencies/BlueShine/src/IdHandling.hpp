
#include <stdio.h>

/* SESSION ID */

#define validateSid() unsigned long sessId = ( unsigned long )sid;\
if ( sessId == 0 ) sessId = lastSession;\
if ( sessId == 0 || sessions.get( sessId - 1 ) == NULL )\
{\
    ShineDB::setLastError( "BlueShine: invalid session id" );\
    return -1;\
}\
lastSession = sessId;\
ShineDB::Session* session = sessions.get( sessId - 1 );

/* RESULT ID */

#define validateRid( onErr ) unsigned long resultId = ( unsigned long )rid;\
ShineDB::QueryOutput* result;\
if ( resultId == 0 )\
    result = lastOutput;\
else if ( results.get( resultId - 1 ) == NULL )\
{\
    ShineDB::setLastError( "BlueShine: invalid result id" );\
    return onErr;\
}\
else result = results.get( resultId - 1 );
