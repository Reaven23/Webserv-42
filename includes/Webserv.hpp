#pragma once

#include "config/Config.hpp"
#include "network/Server.hpp"

#define EPOLL_BLCK_TIME 1000

#define MAX_EVENTS 512

class Webserv {
   private:
    // Default and copy constructor not needed
    Webserv();
    Webserv(Webserv const& other);

    // Operator overload not needed
    Webserv& operator=(Webserv const& ohter);

    // Attributes
    Config const& _config;

    int _epollFd;

    std::vector<Server*> _servers;

    // Methods
    void _runEventLoop() const;

   public:
    // Constructors
    Webserv(Config const& config);

    // Destructor
    ~Webserv();

    // Getters
    std::vector<Server*> const& getServers() const;

    // Public methods
    void start();
};
