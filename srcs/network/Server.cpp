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
Server::Server(int epollFd, ServerConfig const &config)
    : _config(config),
      _epollFd(epollFd),
      _socket(config.getPort()),
      _clients(){};

// Destructor
Server::~Server() { _clear(); };

// Getters
string const &Server::getName() { return (_name); };

int Server::getEpollFd() const { return (_epollFd); };

ServerSocket const &Server::getSocket() { return (_socket); };

int Server::getFd() const { return (_socket.getFd()); };

const map<int, Client *> &Server::getClients() const { return (_clients); };

// Private methods
void Server::removeClient(int clientFd) {
    map<int, Client *>::const_iterator it = _clients.find(clientFd);

    if (it != _clients.end() && it->second) {
        it->second->clear();
        delete it->second;

        _clients.erase(it->first);
    }
}

void Server::_clear() {
    map<int, Client *>::const_iterator it;

    for (it = _clients.begin(); it != _clients.end(); it++) {
        if (it->second) {
            it->second->clear();
            delete it->second;
        }
    }

    _clients.clear();
};

void Server::closeIdleConnections() {
    time_t                             now = time(NULL);
    map<int, Client *>::const_iterator it = _clients.begin();

    while (it != _clients.end()) {
        if (now - it->second->getLastActivity() > KEEP_ALIVE_TIMEOUT) {
            int fd = it->first;
            it++;
            removeClient(fd);
        } else {
            it->second->closeTimeoutCGIs();
            it++;
        }
    }
};

void Server::handleNewClient() {
    int serverFd = getFd();

    // Drain the accept queue in one shot. The server socket is non-blocking;
    // accept returns -1 when the queue is empty, which is our loop exit.
    while (true) {
        Client *client = new Client(_epollFd, &_config);

        client->accept(serverFd);

        int clientFd = client->getFd();

        if (clientFd == -1) {
            delete client;
            return;
        }

        if (!setNonBlocking(clientFd) || !setCloseOnExec(clientFd)) {
            Logger::error(string("fcntl(): ") + strerror(errno));
            delete client;
            continue;
        }

        epoll_event clientEvent = {};
        clientEvent.events = EPOLLIN;
        clientEvent.data.fd = clientFd;

        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
            Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
            close(clientFd);
            delete client;
            continue;
        }

        _clients[clientFd] = client;
    }
};

void Server::handleRequest(int clientFd) {
    map<int, Client *>::iterator it = _clients.find(clientFd);

    if (it == _clients.end() || !it->second) return;

    Client *client = it->second;

    if (client->getState() != Client::NO_CGI) return;

    if (client->read() <= 0) {
        removeClient(clientFd);
        return;
    }

    client->parse();

    if (client->isRequestError()) {
        client->setErrorResponse(400, &_config);
        client->applyVersion();
        client->applyConnectionHeader();
        client->switchToEpollOut();
        client->setLastActivity();
        return;
    }

    if (client->isRequestComplete()) {
        client->logRequest();
        if (client->isCGIRequest(this)) {
            if (client->isSupportedCgi(this)) {
                client->startCGI(this);
                client->setLastActivity();
                return;
            }
            client->setErrorResponse(501, &_config);
            client->applyVersion();
            client->applyConnectionHeader();
            client->switchToEpollOut();
            client->setLastActivity();
        } else {
            client->setResponse();
            client->applyVersion();
            client->applyConnectionHeader();
            client->switchToEpollOut();
            client->setLastActivity();
        }
    }
};

void Server::handleResponse(int clientFd) {
    map<int, Client *>::iterator it = _clients.find(clientFd);

    if (it == _clients.end() || !it->second) return;

    Client *client = it->second;

    if (client->send() <= 0) {
        removeClient(clientFd);
        return;
    }

    if (client->isResponseComplete()) {
        client->logResponse();

        if (client->isKeepAlive()) {
            client->reset();
            client->setLastActivity();

            epoll_event ev = {};
            ev.events = EPOLLIN;
            ev.data.fd = clientFd;

            if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) {
                Logger::error(string("epoll_ctl() (client): Failed to "
                                     "add flag EPOLLIN") +
                              strerror(errno));
            }
        } else
            removeClient(clientFd);
    }
};

void Server::handleCGI(int clientFd, int cgiFd) {
    map<int, Client *>::iterator it = _clients.find(clientFd);

    if (it == _clients.end() || !it->second) return;

    Client *client = _clients[clientFd];

    client->runCGI(cgiFd);
    client->setLastActivity();
}

// Public methods
ServerConfig const &Server::getConfig() { return (_config); }

void Server::setup() {
    int serverFd = getFd();

    epoll_event serverEvent = {};
    serverEvent.events = EPOLLIN;  // Watch read events
    serverEvent.data.fd = serverFd;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverFd, &serverEvent) == -1) {
        throw runtime_error(string("epoll_ctl() (server): Fatal error ") +
                            strerror(errno));
    }

    string const &serverName = _config.getServerName();
    stringstream  ss;

    ss << "Server ";
    serverName.empty() ? ss << "'No name'" : ss << "'" << serverName << "'";
    ss << " listening on port ";
    ss << _socket.getPort();
    Logger::info(ss.str());
}
