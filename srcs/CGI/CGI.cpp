#include "../../includes/CGI/CGI.hpp"

#include "../../includes/utils/Logger.hpp"

// Constructor
CGI::CGI(Server *server) : _server(server){};

// Destructor
CGI::~CGI(){};

// Public method
bool CGI::registerPipe() {
    // register pipe read-end to epoll
    epoll_event cgiEvent = {};
    cgiEvent.events = EPOLLIN;
    cgiEvent.data.fd = _pipe[0];

    if (epoll_ctl(_server->getEpollFd(), EPOLL_CTL_ADD, _pipe[0], &cgiEvent)) {
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        return (false);
    }

    // register pipe read-end to server instance
    _server->getCgis().push_back(_pipe[0]);

    return (true);
};
