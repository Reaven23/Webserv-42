#include <exception>

#include "../includes/config/Config.hpp"
#include "../includes/network/Server.hpp"
#include "../includes/utils/Logger.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return (1);
    }

    Config config;

    config.parse(argv[1]);

    try {
        Server server(config.getServers()[0]);

        server.run();
    } catch (std::exception& e) {
        Logger::error(e.what());
    }

    return (0);
}
