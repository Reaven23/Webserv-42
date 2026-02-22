#pragma once

#include <map>

#include "../config/ServerConfig.hpp"
#include "./Client.hpp"
#include "./ServerSocket.hpp"

#define MAX_EVENTS 10

class Server {
 private:
  // Default constructor is private as it makes no sense to have a server
  // without a config
  Server();
  ServerSocket _socket;

  // Attributes

  std::map<int, Client*> _clients;

  // Methods

  void _remove(int clientFd);

  int _epollCreate() const;
  void _epollAddServer(int epollFd) const;
  void _epollAddClient(int epollFd, int clientFd) const;
  int _epollWait(int epollFd, epoll_event* eventQueue) const;

  void _handleNewClient(int epollFd);

  void _startEventLoop(int epollFd);

 public:
  // Constructors

  Server(ServerConfig& config);

  // Destructor

  ~Server();

  // Getters

  ServerSocket const& getSocket();
  int getFd() const;

  // Methods

  void run();
};
