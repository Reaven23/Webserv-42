#include "../../includes/CGI/CGI.hpp"

#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include "../../includes/network/Client.hpp"

// Constructor
CGI::CGI(Server *server, Client *client)
    : _server(server),
      _client(client),
      _childPid(-1),
      _bodyOffset(0),
      _errorCode(0),
      _lastActivity(time(NULL)) {
    _pipe[0] = -1;
    _pipe[1] = -1;
    _stdinPipe[0] = -1;
    _stdinPipe[1] = -1;
};

// Destructor
CGI::~CGI(){};
