#pragma once

#include <map>

#include "../config/ServerConfig.hpp"
#include "./ServerSocket.hpp"

class Client;

class Server {
   private:
    // Default constructor is private as it makes no sense to have a server
    // without a config
    Server();

    // Attributes
    std::string const _name;

    ServerConfig const&    _config;
    int                    _epollFd;
    ServerSocket           _socket;
    std::map<int, Client*> _clients;

    // Methods
    void _clear();

   public:
    // Constructors
    Server(int epollFd, ServerConfig const& config);

    // Destructor
    ~Server();

    // Getters
    std::string const& getName();

    ServerConfig const& getConfig();

    int getEpollFd() const;

    ServerSocket const& getSocket();
    int                 getFd() const;

    const std::map<int, Client*>& getClients() const;

    // Methods
    void setup();

    void handleNewClient();

    void handleRequest(int clientFd);

    void handleResponse(int clientFd);

    void handleCGI(int clientFd, int cgiFd);

    void removeClient(int clientFd);

    void closeIdleConnections();
};
