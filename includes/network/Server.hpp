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

    // Attributes
    std::string const _name;

    int _epollFd;

    ServerSocket _socket;

    std::map<int, Client*> _clients;

    // Methods

    void _remove(int clientFd);

    void _clear();

    void _handleNewClient();

    void _startEventLoop();

   public:
    // Constructors

    Server(int epollFd, ServerConfig const& config);

    // Destructor

    ~Server();

    // Getters

    std::string const& getName();

    int getEpollFd() const;

    ServerSocket const& getSocket();

    int getFd() const;

    // Methods

    void run();
};
