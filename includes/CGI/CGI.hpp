#pragma once

#include <sys/types.h>

#include <ctime>
#include <string>
#include <vector>

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../network/Server.hpp"

const int CGI_TIMEOUT = 60;

class ServerConfig;

class CGI {
   public:
    // Constructors
    CGI(Server *server, Client *client);

    // Destructors
    ~CGI(void);

    // Getters
    int               *getPipe();
    int                getErrorCode() const;
    const std::string &getScriptPath() const;
    pid_t              getChildPid() const;
    time_t             getLastActivity() const;

    // Setters
    void setLastActivity();

    // Methods
    bool resolvePath(const HttpRequest &request);
    void run(Client *client);
    bool pipe(HttpMethod method);
    void close(int fd);

   private:
    // Attributes
    int         _pipe[2];
    int         _stdinPipe[2];
    Server     *_server;
    Client     *_client;
    pid_t       _childPid;
    size_t      _bodyOffset;
    std::string _scriptPath;
    std::string _resolvedUri;
    int         _errorCode;
    time_t      _lastActivity;

    // Methods
    std::vector<std::string> _buildEnv(const HttpRequest &request);
    HttpResponse             _parseOutput(const std::string &output);
    void _handleWaitingState(Client *client, ServerConfig const *serverConfig,
                             HttpMethod const method);
    void _handleWritingState(Client *client, ServerConfig const *serverConfig);
    void _handleReadingState(Client *client, ServerConfig const *serverConfig);
};
