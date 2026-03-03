#pragma once
#include <string>

#include "Packets.hpp"

namespace phant::util {
    std::string getDefaultServerName();
    ServerIcon detectServerIcon();
}
