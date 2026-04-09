#pragma once
#include "../http/HttpResponse.hpp"
#include "../network/Server.hpp"

class CGI {
   public:
    CGI(Server *server);
    ~CGI(void);
    HttpResponse handleRequest();
    void         pipe();
    int         *getPipe();
    bool         registerPipe();

   private:
    int     _pipe[2];
    Server *_server;
};
