#pragma once

#include "config/Config.hpp"
#include "network/Server.hpp"

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
