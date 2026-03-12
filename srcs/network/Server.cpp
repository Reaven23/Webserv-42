#include "../../includes/network/Server.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
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
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        close(clientFd);
    }

    _clients[clientFd] = client;
};

void Server::_remove(int clientFd) {
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
    epoll_event eventQueue[MAX_EVENTS];

    while (true) {
        int nbEvents = epoll_wait(_epollFd, eventQueue, MAX_EVENTS, -1);
        if (nbEvents == -1) {
            Logger::error(string("epollWait(): ") + strerror(errno));
            break;
        }

        // Loop through events
        for (int i = 0; i < nbEvents; i++) {
            int      fd = eventQueue[i].data.fd;
            uint32_t events = eventQueue[i].events;

            // New connection
            if (fd == _socket.getFd()) _handleNewClient();
            // Data from existing client, request incomplete
            else if (events & EPOLLIN) {
                Client *client = _clients[fd];

                if (client->read() <= 0) continue;

                client->parse();

                if (client->isRequestComplete() || client->isRequestError()) {
                    epoll_event ev = {};
                    ev.events = EPOLLOUT;
                    ev.data.fd = fd;

                    if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                        Logger::error(string("epoll_ctl() (client): Failed to "
                                             "add flag EPOLLOUT ") +
                                      strerror(errno));
                    }
                }
                // Data from existing client, response incomplete
            } else if (events & EPOLLOUT) {
                Client *client = _clients[fd];

                client->isRequestComplete() ? client->setResponse()
                                            : client->setErrorResponse();

                if (client->send() == -1) continue;

                if (client->isResponseComplete()) {
                    // TODO:: if keep-alive
                    // client->reset();
                    // epoll_event ev = {};
                    // ev.events = EPOLLIN;
                    // ev.data.fd = fd;
                    // if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) ==
                    // -1) {
                    //    Logger::error(string("epoll_ctl() (client):
                    //    Failed to " "add flag EPOLLOUT ") +
                    //    strerror(errno));
                    // }
                    // else {}
                    _remove(fd);
                }
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
