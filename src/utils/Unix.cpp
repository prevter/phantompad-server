#include "../Utils.hpp"

#include <fstream>
#include <unistd.h>

namespace phant::util {
    std::string getDefaultServerName() {
        char buf[256];
        if (::gethostname(buf, sizeof(buf)) != 0) {
            return "Unknown PC";
        }
        return buf;
    }

    static bool isSteamDeck() {
        std::ifstream f("/sys/class/dmi/id/product_name");
        if (!f) return false;
        std::string s;
        std::getline(f, s);
        return s == "Jupyter" || s == "Galileo";
    }

    static bool isLaptop() {
        std::ifstream f("/sys/class/power_supply/BAT0/present");
        if (!f) f.open("/sys/class/power_supply/BAT1/present");
        return f.good();
    }

    ServerIcon detectServerIcon() {
        if (isSteamDeck()) return ServerIcon::SteamDeck;
        if (isLaptop()) return ServerIcon::Laptop;
        return ServerIcon::Desktop;
    }
}
