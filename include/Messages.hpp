#pragma once

namespace GameClient
{
    namespace world
    {
        enum
        {
            nop = 0x0080,

            player_movement = 0x2001,
            zone_transition = 0x2002,
            say = 0x2003,
            sync = 0x2004,

            welcome = 0x4001,
            player_list = 0x4002,
            chat_message = 0x4003,
            player_status = 0x4004,
            player_location = 0x4005,
            player_left_area = 0x4006,
            server_message = 0x4007,
            spawn_world_obj = 0x4008,
            sync_rq = 0x4009,
            remove_world_obj = 0x4010,

            FFFF = 0xFFFF
        };
    }

    namespace status
    {
        enum
        {
            offline = 0,
            online = 1,
            afk = 2
        };
    }
}
