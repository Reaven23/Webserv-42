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
    std::vector<int>       _cgis;

    // Methods
    void _remove(int clientFd);
    void _clear();
<<<<<<< HEAD
=======
    void _closeIdleConnections();
    void _handleNewClient();
    void _handleRequest(int clientFd);
    void _handleResponse(int clientFd);
    void _startEventLoop();
>>>>>>> 8ad88154ed510007698c56b7b6ce3b733e1d362f

   public:
    // Constructors
    Server(int epollFd, ServerConfig const& config);

    // Destructor
    ~Server();

    // Getters
<<<<<<< HEAD
    std::string const& getName();

    int getEpollFd() const;

=======
    std::string const&  getName();
    int                 getEpollFd() const;
>>>>>>> 8ad88154ed510007698c56b7b6ce3b733e1d362f
    ServerSocket const& getSocket();
    int                 getFd() const;
    std::vector<int>&   getCgis();

<<<<<<< HEAD
    std::map<int, Client*> getClients();

    // Methods
    void setup();

    void handleNewClient();

    void handleRequest(int Clientfd);

    void handleResponse(int clientFd);

    void closeIdleConnections();
=======
    // Methods
    void run();
>>>>>>> 8ad88154ed510007698c56b7b6ce3b733e1d362f
};
