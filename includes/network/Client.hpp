#pragma once

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/HttpResponse.hpp"

class Client {
   private:
    Client(Client const& other);
    Client& operator=(Client const& other);

    // Attributes
    int               _fd;
    sockaddr_in       _addr;
    std::string       _buffer;
    ParsedHttpRequest _request;
    HttpResponse      _response;

   public:
    // Constructors
    Client();

    // Destructor
    ~Client();

    // Getters
    int                      getFd() const;
    std::string&             getBuffer();
    std::string              getIp() const;
    ParsedHttpRequest const& getRequest();

    // Setters
    void setResponse();
    void setErrorResponse();

    // Methods
    void    accept(int serverFd);
    ssize_t read();
    void    parse();
    ssize_t send();
    bool    isRequestComplete() const;
    bool    isRequestError() const;
    bool    isResponseComplete() const;
};
