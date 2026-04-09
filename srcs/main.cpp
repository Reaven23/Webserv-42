#include <exception>

#include "../includes/Webserv.hpp"
#include "../includes/config/Config.hpp"
#include "../includes/utils/Logger.hpp"

#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        Logger::error("Error: invalid number of arguments");
        return (1);
    }

    Config config;

	try {
		config.parse(argv[1]);
	} catch (std::exception& e) {
		Logger::error("Parsing error: " + std::string(e.what()));
		return (1);
	}

	config.print();////

    try {
        Webserv webserv(config);

        webserv.start();
    } catch (std::exception& e) {
        Logger::error(e.what());
    }

    return (0);
}
