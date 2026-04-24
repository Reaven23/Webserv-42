#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/network/Client.hpp"
#include "../../includes/network/helpers.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

bool CGI::pipe(HttpMethod method) {
    if (::pipe(_pipe) == -1) {
        Logger::error(string("CGI::pipe(): ") + strerror(errno));
        return (false);
    }

    if (!setNonBlocking(_pipe[0])) {
        Logger::error(string("fcntl(): ") + strerror(errno));
        this->close(_pipe[0]);
        this->close(_pipe[1]);
        return (false);
    }

    // register pipe read-end to epoll
    epoll_event cgiReadEvent = {};
    cgiReadEvent.events = EPOLLIN;
    cgiReadEvent.data.fd = _pipe[0];

    if (epoll_ctl(_server->getEpollFd(), EPOLL_CTL_ADD, _pipe[0],
                  &cgiReadEvent)) {
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        this->close(_pipe[0]);
        this->close(_pipe[1]);
        return (false);
    }

    if (!setCloseOnExec(_pipe[0]) || !setCloseOnExec(_pipe[1])) {
        Logger::error(string("fcntl(): ") + strerror(errno));
        this->close(_pipe[0]);
        this->close(_pipe[1]);
        return (false);
    };

    if (method == POST) {
        if (::pipe(_stdinPipe) == -1) {
            Logger::error(string("CGI::pipe(): ") + strerror(errno));
            this->close(_pipe[0]);
            this->close(_pipe[1]);
            return (false);
        }

        if (!setNonBlocking(_stdinPipe[1])) {
            Logger::error(string("fcntl(): ") + strerror(errno));
            this->close(_pipe[0]);
            this->close(_pipe[1]);
            this->close(_stdinPipe[0]);
            this->close(_stdinPipe[1]);
            return (false);
        }

        // register stdinPipe write-end to epoll
        epoll_event cgiWriteEvent = {};
        cgiWriteEvent.events = EPOLLOUT;
        cgiWriteEvent.data.fd = _stdinPipe[1];

        if (epoll_ctl(_server->getEpollFd(), EPOLL_CTL_ADD, _stdinPipe[1],
                      &cgiWriteEvent)) {
            Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
            this->close(_pipe[0]);
            this->close(_pipe[1]);
            this->close(_stdinPipe[0]);
            this->close(_stdinPipe[1]);
            return (false);
        }

        if (!setCloseOnExec(_stdinPipe[0]) || !setCloseOnExec(_stdinPipe[1])) {
            Logger::error(string("fcntl(): ") + strerror(errno));
            this->close(_pipe[0]);
            this->close(_pipe[1]);
            this->close(_stdinPipe[0]);
            this->close(_stdinPipe[1]);
            return (false);
        };
    }

    // register pipes on client instance only after full success
    _client->getCgis()[_pipe[0]] = this;
    if (method == POST) _client->getCgis()[_stdinPipe[1]] = this;

    return (true);
};

void CGI::close(int fd) {
    if (fd >= 0) ::close(fd);
}

void CGI::run(Client *client, int firedFd) {
    ServerConfig const *serverConfig = &_server->getConfig();
    HttpMethod const    method = client->getRequest().request.method;

    if (firedFd == -1)
        _handleWaitingState(client, serverConfig, method);
    else if (firedFd == _pipe[0])
        _handleReadingState(client, serverConfig);
    else if (firedFd == _stdinPipe[1])
        _handleWritingState(client, serverConfig);
    setLastActivity();
}
