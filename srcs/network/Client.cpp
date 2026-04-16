#include "../../includes/network/Client.hpp"

#include <iostream>
#include <sstream>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

bool Client::_endsWith(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
           0;
}

// Constructors
Client::Client(const ServerConfig* serverConfig)
    : _fd(-1),
      _sendBuffer(""),
      _sendOffset(0),
      _lastActivity(time(NULL)),
      _serverConfig(serverConfig) {}

// Destructor
Client::~Client() { close(_fd); };

// Getters
int Client::getFd() const { return (_fd); };

vector<int>& Client::getCgiFds() { return (_cgisFds); };

string& Client::getBuffer() { return (_buffer); };

string Client::getIp() const {
    unsigned char* ip = (unsigned char*)&_addr.sin_addr.s_addr;

    stringstream ss;
    ss << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
       << (int)ip[3];

    return (string(ss.str()));
};

ParsedHttpRequest const& Client::getRequest() { return (_request); };

time_t Client::getLastActivity() const { return (_lastActivity); };

// Setters
void Client::setResponse() {
    _response = HttpResponse::handleRequest(_request.request, _serverConfig);
};

void Client::setErrorResponse() {
    ostringstream cl;
    cl << _response.body.size();

    _response.setStatus(400, "Bad Request")
        .setHeader("Content-Type", "text/plain")
        .setHeader("Content-Length", cl.str())
        .setBody("Bad request");
};

void Client::setNotImplementedResponse() {
    _response =
        IHttpHandler::errorResponse(501, "Not Implemented", _serverConfig);
}

void Client::setCGIResponse(Server* server) {
    CGI cgi(server, this);

    if (!cgi.resolvePath(_request.request)) {
        int         code = cgi.getErrorCode();
        std::string reason = "Bad Request";
        if (code == 404)
            reason = "Not Found";
        else if (code == 403)
            reason = "Forbidden";
        _response = IHttpHandler::errorResponse(code, reason, _serverConfig);
        return;
    }

    if (_request.request.method == GET)
        _response = cgi.executeGet(_request.request);
    else if (_request.request.method == POST)
        _response = cgi.executePost(_request.request);
    else
        _response = IHttpHandler::errorResponse(405, "Method Not Allowed",
                                                _serverConfig);

    setLastActivity();
};

void Client::setLastActivity() { _lastActivity = time(NULL); };

// Public methods

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

    if (bytes > 0)
        setLastActivity();

    return (bytes);
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

void Client::reset() {
    _buffer.erase(0, _request.consumed);
    _sendBuffer = "";
    _sendOffset = 0;

    _request.consumed = 0;
    _request.status = INCOMPLETE;
    _request.request = HttpRequest();

    _response = HttpResponse();
};

bool Client::isRequestComplete() const { return (_request.status == OK); };

bool Client::isRequestError() const { return (_request.status == ERROR); };

bool Client::isResponseComplete() const {
    return (!_sendBuffer.empty() && _sendOffset >= _sendBuffer.size());
};

bool Client::isKeepAlive() const {
    const map<string, string>&          headers = _request.request.headers;
    map<string, string>::const_iterator it = headers.find("connection");

    if (_request.request.version == "HTTP/1.1") {
        if (it != headers.end() && it->second == "close")
            return false;
        return true;
    }
    if (it != headers.end() && it->second == "keep-alive")
        return true;
    return false;
}

void Client::applyConnectionHeader() {
    bool keepAlive = isKeepAlive();
    int  code      = _response.statusCode;

    if (code >= 400)
        keepAlive = false;

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
