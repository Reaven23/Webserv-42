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
    : _name(""),
      _config(config),
      _epollFd(epollFd),
      _socket(config.getPort()),
      _clients(){};

// Destructor
Server::~Server(){};

// Getters
string const &Server::getName() { return (_name); };

int Server::getEpollFd() const { return (_epollFd); };

ServerSocket const &Server::getSocket() { return (_socket); };

int Server::getFd() const { return (_socket.getFd()); };

const map<int, Client *> &Server::getClients() const { return (_clients); };

// Private methods
void Server::_remove(int clientFd) {
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
            _remove(fd);
        } else {
            it->second->closeTimeoutCGIs();
            it++;
        }
    }
};

void Server::handleNewClient() {
    int serverFd = getFd();

    Client *client = new Client(_epollFd, &_config);

    client->accept(serverFd);

    int clientFd = client->getFd();

    if (clientFd == -1) {
        Logger::error(std::string("accept(): Failed to accept connection: ") +
                      strerror(errno));
        delete client;

        return;
    }

    // Set new socket to non-blocking + close-on-exec (pour ne pas leak les fd
    // dans les child CGI)
    if (!setSocketNonBlocking(clientFd) || !setCloseOnExec(clientFd)) {
        Logger::error(string("fcntl(): ") + strerror(errno));
        delete client;
        return;
    }

    // Add new socket to epoll interest list
    epoll_event clientEvent = {};
    clientEvent.events = EPOLLIN;
    clientEvent.data.fd = clientFd;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        close(clientFd);
        delete client;
        return;
    }

    _clients[clientFd] = client;
};

void Server::handleRequest(int clientFd) {
    map<int, Client *>::iterator it = _clients.find(clientFd);
    if (it == _clients.end() || !it->second) return;
    Client *client = it->second;

    if (client->read() <= 0) {
        _remove(clientFd);
        return;
    }

    client->parse();

    if (client->isRequestError()) {
        client->setErrorResponse(400, &_config);
        client->switchToEpollOut();
        client->setLastActivity();
        return;
    }

    if (client->isRequestComplete()) {
        if (client->isCGIRequest(this)) {
            if (client->isSupportedCgi(this)) {
                client->initCGI(this);
                client->setLastActivity();
                return;
            }
            client->setErrorResponse(501, &_config);
            client->switchToEpollOut();
            client->setLastActivity();
        } else {
            client->setResponse();
            client->switchToEpollOut();
            client->setLastActivity();
        }
    }
};

void Server::handleResponse(int clientFd) {
    map<int, Client *>::iterator it = _clients.find(clientFd);
    if (it == _clients.end() || !it->second) return;
    Client *client = it->second;

    if (client->send() == -1) {
        _remove(clientFd);
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
            _remove(clientFd);
    }
};

void Server::handleCGI(int clientFd, int cgiFd) {
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

    stringstream ss;

    ss << "Server ";
    _name.empty() ? ss << "'No name'" : ss << "'" << _name << "'";
    ss << " listening on port ";
    ss << _socket.getPort();
    Logger::info(ss.str());
}
