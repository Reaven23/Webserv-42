#pragma once

#include <map>

#include "../config/ServerConfig.hpp"
#include "./Client.hpp"
#include "./ServerSocket.hpp"

#define MAX_EVENTS 10

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
    void _remove(int clientFd);
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

    void handleRequest(int Clientfd);

    void handleResponse(int clientFd);

    void closeIdleConnections();
};
