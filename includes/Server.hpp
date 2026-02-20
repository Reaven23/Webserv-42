#pragma once

#include <sstream>

#include "ServerSocket.hpp"
#include "utils/Logger.hpp"

class Server {
 private:
  // Default constructor is private as it makes no sense to have a server
  // without a port
  Server();
  ServerSocket _socket;

 public:
  Server(int port) : _socket(port) {
    stringstream ss;

    ss << _socket.getPort();
    Logger::info(string("Server listening on port ") + ss.str());
  };

  ~Server(){};

  ServerSocket const& getSocket() { return (_socket); };
  int getSocketFd() const { return (_socket.getFd()); }

  // TODO: ajouter la mecanique d'ecoute avec epoll dans cette methode
  // void run();
};
