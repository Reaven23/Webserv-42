#pragma once

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <string>

#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/HttpResponse.hpp"

const int KEEP_ALIVE_TIMEOUT = 60;

class ServerConfig;

class Client {
   private:
    Client(Client const& other);
    Client& operator=(Client const& other);

    // Attributes
    int                 _fd;
    sockaddr_in         _addr;
    std::string         _buffer;
    ParsedHttpRequest   _request;
    HttpResponse        _response;
    time_t              _lastActivity;
    const ServerConfig* _serverConfig;

   public:
    // Constructors
    Client(const ServerConfig* serverConfig = 0);

    // Destructor
    ~Client();

    // Getters
    int                      getFd() const;
    std::string&             getBuffer();
    std::string              getIp() const;
    ParsedHttpRequest const& getRequest();
    time_t                   getLastActivity() const;

    // Setters
    void setResponse();
    void setErrorResponse();
    void setLastActivity();

    // Methods
    void    accept(int serverFd);
    ssize_t read();
    void    parse();
    ssize_t send();
    void    reset();
    bool    isRequestComplete() const;
    bool    isRequestError() const;
    bool    isResponseComplete() const;
    bool    isKeepAlive() const;
    void    logResponse() const;
    // void handleCGI(Server *server);
};
