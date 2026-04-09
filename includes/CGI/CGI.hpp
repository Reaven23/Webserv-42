#pragma once

#include <string>
#include <vector>

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../network/Server.hpp"

class ServerConfig;

class CGI {
   public:
    CGI(Server *server, Client *client);
    ~CGI(void);

    bool               resolvePath(const HttpRequest &request);
    int                getErrorCode() const;
    const std::string &getScriptPath() const;

    HttpResponse executeGet(const HttpRequest &request);
    HttpResponse executePost(const HttpRequest &request);

    bool pipe();
    void close(int fd);
    int *getPipe();

   private:
    int         _pipe[2];
    Server     *_server;
    Client     *_client;
    std::string _scriptPath;
    std::string _resolvedUri;
    int         _errorCode;

    std::vector<std::string> _buildEnv(const HttpRequest &request);
    HttpResponse             _parseOutput(const std::string &output);
    HttpResponse             _execute(const HttpRequest &request,
                                      const std::string &body);
};
