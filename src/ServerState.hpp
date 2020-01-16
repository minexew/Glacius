#ifndef GLACIUS_SERVERSTATE_HPP
#define GLACIUS_SERVERSTATE_HPP

#include <string>

namespace Glacius {

enum class ServerState {
    up,
    down,
};

struct ServerStateChange {
    ServerState state;
    std::string reason;
};

}

#endif
