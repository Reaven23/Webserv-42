#include "../../includes/network/Server.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>

#include "../../includes/network/Client.hpp"
#include "../../includes/network/helpers.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

// Constructors

// TODO: passer 'server_name' depuis la config pour _name
Server::Server(int epollFd, ServerConfig const &config)
    : _name(""), _epollFd(epollFd), _socket(config.getPort()), _clients(){};

// Destructor

Server::~Server(){};

// Getters

string const &Server::getName() { return (_name); };

int Server::getEpollFd() const { return (_epollFd); };

ServerSocket const &Server::getSocket() { return (_socket); };

int Server::getFd() const { return (_socket.getFd()); };

// Private methods

void Server::_handleNewClient() {
    int serverFd = getFd();

    Client *client = new Client();

    client->accept(serverFd);

    int clientFd = client->getFd();

    if (clientFd == -1) {
        if (errno == EAGAIN ||
            errno == EWOULDBLOCK) {  // All pending connections processed
            Logger::info("All incoming connections processed");

            return;
        } else {
            Logger::error(
                std::string("accept(): Failed to accept connection: ") +
                strerror(errno));
        }
        delete client;

        return;
    }

    // Set new socket to non-blocking
    if (!setSocketNonBlocking(clientFd)) {
        Logger::error(string("fcntl(): ") + strerror(errno));

        return;
    }

    // Add new socket to epoll interest list
    epoll_event clientEvent = {};
    clientEvent.events = EPOLLIN;
    clientEvent.data.fd = clientFd;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
        Logger::error(string("epollAddClient() (client): ") + strerror(errno));
        close(clientFd);
    }

    _clients[clientFd] = client;
};

void Server::_erase(int clientFd) {
    map<int, Client *>::const_iterator it = _clients.find(clientFd);

    if (it != _clients.end() && it->second) {
        delete it->second;

        _clients.erase(it->first);
    }
}

void Server::_clear() {
    map<int, Client *>::const_iterator it;

    for (it = _clients.begin(); it != _clients.end(); it++) {
        if (it->second) {
            delete it->second;
        }
    }

    _clients.clear();
};

void Server::_startEventLoop() {
    int serverFd = getFd();

    epoll_event eventQueue[MAX_EVENTS];

    while (true) {
        int nbEvents = epoll_wait(_epollFd, eventQueue, MAX_EVENTS, -1);
        if (nbEvents == -1) {
            Logger::error(string("epollWait(): ") + strerror(errno));
            break;
        }

        // Loop through events
        for (int i = 0; i < nbEvents; i++) {
            if (eventQueue[i].data.fd == serverFd)  // New connection
                _handleNewClient();
            else {  // Data from existing client
                Client *client = _clients[eventQueue[i].data.fd];

                client->read();

                /* if (client->isRequestComplete()) {
                  // TODO: traiter la requete (a voir avec Adrien)
                  // HttpRequest request;
                  // request.parse(client->getBuffer());
                  // if (request.getMethod() === GET)
                  //  ....

                  _remove(client->getFd());

                  delete client;
                } */
            }
        }
    }
}

// Public methods

void Server::run() {
    int serverFd = getFd();

    epoll_event serverEvent = {};
    serverEvent.events = EPOLLIN;  // Watch read events
    serverEvent.data.fd = serverFd;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverFd, &serverEvent) == -1) {
        throw runtime_error(string("epollAddServer() (server): Fatal error ") +
                            strerror(errno));
    }

    stringstream ss;

    ss << "Server ";
    _name.empty() ? ss << "'No name'" : ss << "'" << _name << "'";
    ss << " listening on port ";
    ss << _socket.getPort();
    Logger::info(ss.str());

    _startEventLoop();
}
