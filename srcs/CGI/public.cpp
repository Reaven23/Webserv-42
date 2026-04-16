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
#include "../../includes/utils/Logger.hpp"

using namespace std;

bool CGI::pipe(HttpMethod method) {
    if (::pipe(_pipe) == -1) {
        Logger::error(string("CGI::pipe(): ") + strerror(errno));
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

    // register pipe read-end to client instance
    _client->getCgis()[_pipe[0]] = this;

    if (method == POST) {
        if (::pipe(_stdinPipe) == -1) {
            Logger::error(string("CGI::pipe(): ") + strerror(errno));
            this->close(_pipe[0]);
            this->close(_pipe[1]);
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

        // register stdinPipe write-end to client instance
        _client->getCgis()[_stdinPipe[1]] = this;
    }

    return (true);
};

void CGI::close(int fd) {
    map<int, CGI *>          &cgis = _client->getCgis();
    map<int, CGI *>::iterator it = cgis.find(fd);

    if (it != cgis.end()) {
        cgis.erase(it);
    }

    if (fd >= 0) ::close(fd);
}

void CGI::run(Client *client) {
    Client::State       state = client->getState();
    ServerConfig const *serverConfig = &_server->getConfig();
    HttpMethod const    method = client->getRequest().request.method;

    if (state == Client::WAITING_CGI) {
        _handleWaitingState(client, serverConfig, method);
    } else if (state == Client::WRITING_CGI) {
        _handleWritingState(client, serverConfig);
    } else if (state == Client::READING_CGI) {
        _handleReadingState(client, serverConfig);
    }
}
