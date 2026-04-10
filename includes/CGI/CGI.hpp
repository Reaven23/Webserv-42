#pragma once

#include <string>

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../network/Server.hpp"

class CGI {
   public:
    CGI(Server *server, Client *client);
    ~CGI(void);

    bool               resolvePath(const HttpRequest &request);
    int                getErrorCode() const;
    const std::string &getScriptPath() const;

    // Methods
    HttpResponse handleRequest();
    bool         pipe();
    void         close(int fd);

    // Getters
    int *getPipe();

   private:
    int         _pipe[2];
    Server     *_server;
    Client     *_client;
    std::string _scriptPath;
    int         _errorCode;
};
