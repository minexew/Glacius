#pragma once

#ifdef _WIN32
#define BlueShine extern "C" __declspec( dllimport )
#else
#define BlueShine
#endif

BlueShine double BS_connect( const char* host, const char* user, const char* pass, const char* db );
BlueShine const char* BS_error();
BlueShine double BS_close( double sid );
BlueShine double BS_query( double sid, const char* query );
BlueShine double BS_affected_rows( double sid );
BlueShine double BS_last_insert_id( double sid );
BlueShine double BS_store_result();
BlueShine double BS_num_fields( double rid );
BlueShine const char* BS_get_field( double rid, double index );
BlueShine double BS_num_rows( double rid );
BlueShine const char* BS_get_cell_by_name( double rid, double r, const char* name );
BlueShine const char* BS_get_cell_by_index( double rid, double r, double index );
BlueShine double BS_free_result( double rid );
