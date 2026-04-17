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
#include "./Server.hpp"

const int KEEP_ALIVE_TIMEOUT = 60;

class ServerConfig;
class Server;
class LocationConfig;

class Client {
   private:
    Client(Client const& other);
    Client& operator=(Client const& other);

    static bool _endsWith(const std::string& value, const std::string& suffix);

    // Attributes
    int                 _fd;
    std::vector<int>    _cgisFds;
    sockaddr_in         _addr;
    std::string         _buffer;
    ParsedHttpRequest   _request;
    HttpResponse        _response;
    std::string         _sendBuffer;
    size_t              _sendOffset;
    time_t              _lastActivity;
    const ServerConfig* _serverConfig;

   public:
    // Constructors
    Client(const ServerConfig* serverConfig = 0);

    // Destructor
    ~Client();

    // Getters
    int                      getFd() const;
    std::vector<int>&        getCgiFds();
    std::string&             getBuffer();
    std::string              getIp() const;
    ParsedHttpRequest const& getRequest();
    time_t                   getLastActivity() const;

    // Setters
    void setResponse();
    void setErrorResponse();
    void setNotImplementedResponse();
    void setCGIResponse(Server* server);
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
    void    applyVersion();
    void    applyConnectionHeader();
    bool    isCGIRequest(Server* server) const;
    bool    isSupportedCgi(Server* server) const;
    void    logResponse() const;
};
