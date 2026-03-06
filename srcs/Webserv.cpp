#include "../includes/Webserv.hpp"

#include <stdexcept>
#include <string>

#include "../includes/config/ServerConfig.hpp"
#include "../includes/network/Server.hpp"

using namespace std;

// Constructors
Webserv::Webserv(Config const& config) : _config(config) {}

// Destructor
Webserv::~Webserv() {
    vector<Server*>::const_iterator it;

    for (it = _servers.begin(); it != _servers.end(); it++) {
        delete *it;
    }
};

// Getters
std::vector<Server*> const& Webserv::getServers() const { return (_servers); };

// Public methods
void Webserv::start() {
    int epollFd = epoll_create(1);

    if (epollFd == -1) {
        throw runtime_error(string("epollCreate(): Fatal error: ") +
                            strerror(errno));
    }

    std::vector<ServerConfig> const& serversConfig = _config.getServers();
    std::vector<ServerConfig>::const_iterator it;

    for (it = serversConfig.begin(); it != serversConfig.end(); it++) {
        Server* server = new Server(epollFd, *it);

        _servers.push_back(server);

        server->run();
    }
};
