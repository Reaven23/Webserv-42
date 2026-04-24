#include "../../includes/network/Client.hpp"

#include <sys/wait.h>

#include <csignal>
#include <iostream>
#include <sstream>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

bool Client::_endsWith(const string& value, const string& suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
           0;
}

void Client::_removeCGI(int cgiFd) {
    map<int, CGI*>::const_iterator it = _cgis.find(cgiFd);

    if (it != _cgis.end() && it->second) {
        pid_t childPid = it->second->getChildPid();

        kill(childPid, SIGKILL);
        waitpid(childPid, NULL, 0);
        delete it->second;
        _cgis.erase(it->first);
    }
}

void Client::_finalizeCGI(CGI* cgi) {
    if (cgi == NULL) return;
    map<int, CGI*>::iterator it = _cgis.begin();
    while (it != _cgis.end()) {
        if (it->second == cgi) {
            map<int, CGI*>::iterator tmp = it++;
            _cgis.erase(tmp);
        } else {
            ++it;
        }
    }
    delete cgi;
}

// Constructors
Client::Client(int epollFd, const ServerConfig* serverConfig)
    : _fd(-1),
      _epollFd(epollFd),
      _state(NO_CGI),
      _sendBuffer(""),
      _sendOffset(0),
      _lastActivity(time(NULL)),
      _serverConfig(serverConfig),
      _cgiBuffer("") {}

// Destructor
Client::~Client() {
    clear();
    close(_fd);
};

// Getters
int Client::getFd() const { return (_fd); };

int Client::getEpollFd() const { return (_epollFd); }

Client::State Client::getState() const { return (_state); }

string& Client::getBuffer() { return (_buffer); };

string Client::getIp() const {
    unsigned char* ip = (unsigned char*)&_addr.sin_addr.s_addr;

    stringstream ss;
    ss << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
       << (int)ip[3];

    return (string(ss.str()));
};

ParsedHttpRequest const& Client::getRequest() { return (_request); };

map<int, CGI*>& Client::getCgis() { return (_cgis); };

string& Client::getCgiBuffer() { return (_cgiBuffer); };

HttpResponse& Client::getResponse() { return (_response); }

time_t Client::getLastActivity() const { return (_lastActivity); };

// Setters
void Client::setResponse() {
    _response = HttpResponse::handleRequest(_request.request, _serverConfig);
    _state = SENDING_RESPONSE;
};

void Client::setErrorResponse(int statusCode, ServerConfig const* config) {
    string reason = "";
    switch (statusCode) {
        case 400:
            reason = "Bad Request";
            break;
        case 403:
            reason = "Forbidden";
            break;
        case 404:
            reason = "Not Found";
            break;
        case 405:
            reason = "Method Not Allowed";
            break;
        case 501:
            reason = "Not Implemented";
            break;
        case 504:
            reason = "Gateway Timeout";
            break;
        case 500:
        default:
            reason = "Internal server error";
            break;
    }

    _response = IHttpHandler::errorResponse(statusCode, reason, config);
    _state = SENDING_RESPONSE;
};

void Client::setState(Client::State state) { _state = state; }

void Client::setLastActivity() { _lastActivity = time(NULL); };

// Public methods
void Client::startCGI(Server* server) {
    CGI* cgi = new CGI(server, this);

    this->setState(WAITING_CGI);
    if (!cgi->resolvePath(_request.request)) {
        setErrorResponse(cgi->getErrorCode(), _serverConfig);
        applyVersion();
        applyConnectionHeader();
        switchToEpollOut();
        delete cgi;
        return;
    }

    if (!cgi->pipe(_request.request.method)) {
        delete cgi;
        setErrorResponse(500, _serverConfig);
        applyVersion();
        applyConnectionHeader();
        setState(Client::SENDING_RESPONSE);
        switchToEpollOut();
        return;
    }

    cgi->run(this, -1);

    if (_state == SENDING_RESPONSE) _finalizeCGI(cgi);
};

void Client::accept(int serverFd) {
    socklen_t addrLen = sizeof(_addr);

    _fd = ::accept(serverFd, (struct sockaddr*)&_addr, &addrLen);
}

ssize_t Client::read() {
    ostringstream oss;

    char    tmp[1024] = {0};
    ssize_t bytes = recv(_fd, tmp, sizeof(tmp), 0);

    if (bytes > 0) {
        _buffer.append(tmp, bytes);
        setLastActivity();
    } else if (bytes == 0) {
        oss << "Client '" << _fd << "|" << getIp()
            << "' has ended the connection" << endl;
        Logger::info(oss.str());
    } else {
        oss << "recv(): transient error on client '" << _fd << "|" << getIp()
            << "'";
        Logger::error(oss.str());
        return (1);
    }

    return (bytes);
}

void Client::logRequest() const {
    string const& serverName = _serverConfig->getServerName();
    ostringstream oss;

    oss << "Server ";
    serverName.empty() ? oss << "'No name'" : oss << "'" << serverName << "'";
    oss << " received request from client '" << _fd << "|" << getIp() << "'";
    Logger::info(oss.str());
}

void Client::runCGI(int cgiFd) {
    map<int, CGI*>::iterator it = _cgis.find(cgiFd);
    if (it == _cgis.end() || it->second == NULL) return;
    CGI* cgi = it->second;

    if (_request.request.method != GET && _request.request.method != POST) {
        applyVersion();
        applyConnectionHeader();
        setErrorResponse(405, _serverConfig);
        switchToEpollOut();
        return;
    }

    cgi->run(this, cgiFd);

    if (_state == SENDING_RESPONSE) _finalizeCGI(cgi);
}

void Client::parse() { _request = HttpRequest::parse(_buffer); }

ssize_t Client::send() {
    ostringstream os;

    if (_state != SENDING_RESPONSE) return (-1);

    if (_sendBuffer.empty()) {
        _sendBuffer = _response.toString();
    }

    ssize_t bytes = ::send(_fd, _sendBuffer.c_str() + _sendOffset,
                           _sendBuffer.size() - _sendOffset, 0);

    if (bytes < 0) {
        os << "send(): transient error on client '" << _fd << "|" << getIp()
           << "'";
        Logger::error(os.str());
        return (1);
    }
    if (bytes == 0) {
        os << "send(): connection closed by peer '" << _fd << "|" << getIp()
           << "'";
        Logger::error(os.str());
        return (-1);
    }

    _sendOffset += bytes;
    setLastActivity();

    return (bytes);
}

void Client::clear() {
    map<int, CGI*>::const_iterator it;

    for (it = _cgis.begin(); it != _cgis.end(); ++it) {
        CGI* cgi = it->second;
        if (cgi == NULL) continue;
        map<int, CGI*>::const_iterator scan = _cgis.begin();
        bool                           alreadySeen = false;
        for (; scan != it; ++scan) {
            if (scan->second == cgi) {
                alreadySeen = true;
                break;
            }
        }
        if (!alreadySeen) delete cgi;
    }

    _cgis.clear();
}

void Client::reset() {
    _buffer.erase(0, _request.consumed);

    _cgiBuffer.clear();
    _state = NO_CGI;

    _sendBuffer = "";
    _sendOffset = 0;

    _request.consumed = 0;
    _request.status = INCOMPLETE;
    _request.request = HttpRequest();

    _response = HttpResponse();
};

void Client::switchToEpollOut() const {
    epoll_event event = {};
    event.events = EPOLLOUT;
    event.data.fd = getFd();
    if (epoll_ctl(getEpollFd(), EPOLL_CTL_MOD, getFd(), &event) == -1) {
        Logger::error(string("epoll_ctl() (client): Failed to "
                             "add flag EPOLLOUT ") +
                      strerror(errno));
    }
};

void Client::appendToCGIBuffer(char const* buffer, ssize_t bytes) {
    _cgiBuffer.append(buffer, bytes);
}

bool Client::isRequestComplete() const { return (_request.status == OK); };

bool Client::isRequestError() const { return (_request.status == ERROR); };

bool Client::isResponseComplete() const {
    return (!_sendBuffer.empty() && _sendOffset >= _sendBuffer.size());
};

bool Client::isKeepAlive() const {
    const map<string, string>&          headers = _request.request.headers;
    map<string, string>::const_iterator it = headers.find("connection");

    if (_request.request.version == "HTTP/1.1") {
        if (it != headers.end() && it->second == "close") return false;
        return true;
    }
    if (it != headers.end() && it->second == "keep-alive") return true;
    return false;
}

void Client::applyVersion() {
    const string& ver = _request.request.version;
    if (ver == "HTTP/1.0" || ver == "HTTP/1.1") _response.version = ver;
}

void Client::applyConnectionHeader() {
    bool keepAlive = isKeepAlive();
    int  code = _response.statusCode;

    if (code >= 400) keepAlive = false;

    if (keepAlive)
        _response.setHeader("Connection", "keep-alive");
    else
        _response.setHeader("Connection", "close");
}

bool Client::isCGIRequest(Server* server) const {
    if (server == 0 || _request.status != OK || _request.request.uri.empty())
        return false;
    string clean;
    int    err = 0;
    if (!IHttpHandler::sanitizeUriPath(_request.request.uri, clean, err))
        return false;
    const LocationConfig* location =
        IHttpHandler::findLocation(clean, &server->getConfig());
    if (location == 0 || !location->hasCgiExtension()) return false;
    return _endsWith(clean, location->getCgiExtension());
}

bool Client::isSupportedCgi(Server* server) const {
    if (server == 0) return false;
    string clean;
    int    err = 0;
    if (!IHttpHandler::sanitizeUriPath(_request.request.uri, clean, err))
        return false;
    const LocationConfig* location =
        IHttpHandler::findLocation(clean, &server->getConfig());
    if (location == 0) return false;

    return location->hasCgiExtension();
}

void Client::logResponse() const {
    ostringstream                       os;
    map<string, string>::const_iterator it;
    string const& serverName = _serverConfig->getServerName();

    os << "Server ";
    serverName.empty() ? os << "'No name'" : os << "'" << serverName << "'";
    os << " sent response to '" << _fd << "|" << getIp() << ":\n";
    os << "{\n";
    os << "  status: " << _response.statusCode << "\n";
    os << "  headers: {\n";
    for (it = _response.headers.begin(); it != _response.headers.end(); it++) {
        os << "    " << it->first << ": " << it->second << "\n";
    }
    os << "  }\n";
    os << "}";

    _response.statusCode >= 200 && _response.statusCode < 400
        ? Logger::info(os.str())
        : Logger::warn(os.str());
}

void Client::closeTimeoutCGIs() {
    time_t                         now = time(NULL);
    map<int, CGI*>::const_iterator it = _cgis.begin();
    bool                           timedOut = false;

    while (it != _cgis.end()) {
        if (now - it->second->getLastActivity() > CGI_TIMEOUT) {
            int fd = it->first;
            it++;
            _removeCGI(fd);
            timedOut = true;
        } else
            it++;
    }

    if (timedOut && _state != SENDING_RESPONSE && _state != NO_CGI) {
        setErrorResponse(504, _serverConfig);
        applyVersion();
        applyConnectionHeader();
        setState(SENDING_RESPONSE);
        switchToEpollOut();
    }
}
