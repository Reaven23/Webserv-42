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
#include "../CGI/CGI.hpp"

const int KEEP_ALIVE_TIMEOUT = 60;

class ServerConfig;
class Server;
class LocationConfig;

class Client {
   public:
    enum State {
        WAITING_CGI,
        WRITING_CGI,
        READING_CGI,
        SENDING_RESPONSE,
    };

    // Constructors
    Client(int epollFd, const ServerConfig* serverConfig = 0);

    // Destructor
    ~Client();

    // Getters
    int                      getFd() const;
    int                      getEpollFd() const;
    State                    getState() const;
    std::string&             getBuffer();
    std::string              getIp() const;
    ParsedHttpRequest const& getRequest();
    HttpResponse&            getResponse();
    std::map<int, CGI*>&     getCgis();
    std::string&             getCgiBuffer();
    time_t                   getLastActivity() const;

    // Setters
    void setResponse();
    void setErrorResponse(int statusCode, ServerConfig const* config);
    void initCGI(Server* server);
    void setLastActivity();
    void setState(State state);

    // Methods
    void    accept(int serverFd);
    ssize_t read();
    void    runCGI(int cgiFd);
    void    parse();
    ssize_t send();
    void    clear();
    void    reset();
    void    switchToEpollOut() const;
    void    appendToCGIBuffer(std::string buffer, ssize_t bytes);
    bool    isRequestComplete() const;
    bool    isRequestError() const;
    bool    isResponseComplete() const;
    bool    isKeepAlive() const;
    bool    isCGIRequest(Server* server) const;
    bool    isSupportedCgi(Server* server) const;
    void    logResponse() const;
    void    applyVersion();
    void    applyConnectionHeader();

   private:
    Client(Client const& other);
    Client& operator=(Client const& other);

    static bool _endsWith(const std::string& value, const std::string& suffix);

    // Attributes
    int                 _fd;
    int                 _epollFd;
    State               _state;
    sockaddr_in         _addr;
    std::string         _buffer;
    ParsedHttpRequest   _request;
    HttpResponse        _response;
    std::string         _sendBuffer;
    size_t              _sendOffset;
    time_t              _lastActivity;
    const ServerConfig* _serverConfig;
    // CGI
    std::map<int, CGI*> _cgis;
    std::string         _cgiBuffer;
};
