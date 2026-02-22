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

Server::Server(ServerConfig &config) : _socket(config.getPort()), _clients() {
  stringstream ss;

  ss << _socket.getPort();
  Logger::info(string("Server listening on port ") + ss.str());
};

// Destructor

Server::~Server(){};

// Getters

ServerSocket const &Server::getSocket() { return (_socket); };

int Server::getFd() const { return (_socket.getFd()); };

// Private methods

int Server::_epollCreate() const {
  int epollFd = epoll_create(1);
  if (epollFd == -1) {
    throw runtime_error(string("epollCreate(): Fatal error: ") +
                        strerror(errno));
  }

  return (epollFd);
};

void Server::_epollAddServer(int epollFd) const {
  int serverFd = getFd();

  epoll_event event = {};
  event.events = EPOLLIN;  // Watch read events
  event.data.fd = serverFd;

  if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1) {
    throw runtime_error(string("epollAddServer() (server): Fatal error ") +
                        strerror(errno));
  }
};

void Server::_epollAddClient(int epollFd, int clientFd) const {
  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = clientFd;

  if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1) {
    Logger::error(string("epollAddClient() (client): ") + strerror(errno));
    close(clientFd);
  }
}

int Server::_epollWait(int epollFd, epoll_event *eventQueue) const {
  int nbEvents = epoll_wait(epollFd, eventQueue, MAX_EVENTS, -1);
  if (nbEvents == -1) {
    Logger::error(string("epollWait(): ") + strerror(errno));
  }

  return (nbEvents);
}

void Server::_handleNewClient(int epollFd) {
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
      Logger::error(std::string("accept(): Failed to accept connection: ") +
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
  _epollAddClient(epollFd, clientFd);

  _clients[clientFd] = client;
};

void Server::_remove(int clientFd) { _clients.erase(clientFd); }

void Server::_startEventLoop(int epollFd) {
  int serverFd = getFd();

  epoll_event eventQueue[MAX_EVENTS];

  while (true) {
    int nbEvents = _epollWait(epollFd, eventQueue);

    if (nbEvents == -1) {
      break;
    }

    // Loop through events
    for (int i = 0; i < nbEvents; i++) {
      if (eventQueue[i].data.fd == serverFd)  // New connection
        _handleNewClient(epollFd);
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
  // This part may throw (fatal errors only)
  int epollFd = _epollCreate();

  _epollAddServer(epollFd);

  // This part only logs errors
  _startEventLoop(epollFd);
}
