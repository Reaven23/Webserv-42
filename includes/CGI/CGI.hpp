#pragma once

#include <string>

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../network/Server.hpp"

class CGI {
   public:
    CGI(Server *server);
    ~CGI(void);

    bool                resolvePath(const HttpRequest &request);
    int                 getErrorCode() const;
    const std::string  &getScriptPath() const;

    HttpResponse handleRequest();
    void         pipe();
    int         *getPipe();
    bool         registerPipe();

   private:
    int         _pipe[2];
    Server     *_server;
    std::string _scriptPath;
    int         _errorCode;
};
