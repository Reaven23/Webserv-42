#include "../../includes/network/Client.hpp"

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

// Constructors
Client::Client(int epollFd, const ServerConfig* serverConfig)
    : _fd(-1),
      _epollFd(epollFd),
      _state(WAITING_CGI),
      _sendBuffer(""),
      _sendOffset(0),
      _lastActivity(time(NULL)),
      _serverConfig(serverConfig),
      _cgiBuffer("") {}

// Destructor
Client::~Client() { close(_fd); };

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
        case 500:
        default:
            reason = "Internal server error";
            break;
    }

    _response = IHttpHandler::errorResponse(statusCode, reason, config);
};

void Client::setState(Client::State state) { _state = state; }

void Client::setLastActivity() { _lastActivity = time(NULL); };

// Public methods
void Client::initCGI(Server* server) {
    CGI* cgi = new CGI(server, this);

    if (!cgi->resolvePath(_request.request)) {
        setErrorResponse(cgi->getErrorCode(), _serverConfig);
        switchToEpollOut();
        delete cgi;
        return;
    }

    if (!cgi->pipe(_request.request.method)) {
        delete cgi;
        setErrorResponse(500, _serverConfig);
        setState(Client::SENDING_RESPONSE);
        switchToEpollOut();
    }
};

void Client::accept(int serverFd) {
    socklen_t addrLen = sizeof(_addr);

    _fd = ::accept(serverFd, (struct sockaddr*)&_addr, &addrLen);
}

ssize_t Client::read() {
    ostringstream oss;

    oss << "Request from client '" << _fd << "|" << getIp() << "'";
    Logger::info(oss.str());

    char    tmp[1024] = {0};
    ssize_t bytes = recv(_fd, tmp, sizeof(tmp), 0);

    if (bytes > 0) {
        _buffer.append(tmp, bytes);
    } else if (bytes == 0) {
        oss.str("");
        oss.clear();

        oss << "Client '" << _fd << "|" << getIp()
            << "' has ended the connection" << endl;
        Logger::info(oss.str());
    } else {
        oss.str("");
        oss.clear();

        oss << "recv(): Failed to read data from client '" << _fd << "|"
            << getIp() << "'";
        oss << strerror(errno);

        Logger::error(oss.str());
    }

    if (bytes > 0) setLastActivity();

    return (bytes);
}

void Client::runCGI(int cgiFd) {
    CGI* cgi = _cgis[cgiFd];

    if (_request.request.method != GET && _request.request.method != POST) {
        setErrorResponse(405, _serverConfig);
        switchToEpollOut();
        return;
    }

    cgi->run(this);
}

void Client::parse() { _request = HttpRequest::parse(_buffer); }

ssize_t Client::send() {
    ostringstream os;

    if (_sendBuffer.empty()) {
        _sendBuffer = _response.toString();
    }

    ssize_t bytes = ::send(_fd, _sendBuffer.c_str() + _sendOffset,
                           _sendBuffer.size() - _sendOffset, 0);

    if (bytes == -1) {
        os << "send(): Failed to send data to client '" << _fd << "|" << getIp()
           << "'" << endl;
        os << strerror(errno);

        Logger::error(os.str());
        return (-1);
    }

    _sendOffset += bytes;

    setLastActivity();

    return (bytes);
}

void Client::clear() {
    map<int, CGI*>::const_iterator it;

    for (it = _cgis.begin(); it != _cgis.end(); it++) {
        if (it->second) delete it->second;
    }

    _cgis.clear();
}

void Client::reset() {
    _buffer.erase(0, _request.consumed);

    _cgiBuffer.clear();
    _state = WAITING_CGI;

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

void Client::appendToCGIBuffer(string buffer, ssize_t bytes) {
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
    return location->getCgiExtension() == ".py";
}

void Client::logResponse() const {
    ostringstream                       os;
    map<string, string>::const_iterator it;

    os << "Response sent to '" << _fd << "|" << getIp() << ":\n";
    os << "{\n";
    os << "  status: " << _response.statusCode << "\n";
    os << "  headers: {\n";
    for (it = _response.headers.begin(); it != _response.headers.end(); it++) {
        os << "    " << it->first << ": " << it->second << "\n";
    }
    os << "  }\n";
    os << "}";

    _response.statusCode >= 200 && _response.statusCode < 300
        ? Logger::info(os.str())
        : Logger::warn(os.str());
}
