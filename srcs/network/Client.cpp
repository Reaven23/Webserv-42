#include "../../includes/network/Client.hpp"

#include <sys/types.h>

#include <iostream>
#include <sstream>

#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

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
        .setHeader("Connection", "close")
        .setBody("Bad request");
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
    } else if (bytes == 0) {  // client has ended the connection
        oss.str("");
        oss.clear();

        oss << "Client '" << _fd << "|" << getIp()
            << "' has ended the connection" << endl;
        Logger::info(oss.str());

        close(_fd);  // automatically removes the fd from epoll interest list;

    } else {
        oss.str("");
        oss.clear();

        oss << "recv(): Failed to read data from client '" << _fd << "|"
            << getIp() << "'";
        oss << strerror(errno);

        Logger::error(oss.str());
        close(_fd);
    }

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
    map<string, string>                 headers = _request.request.headers;
    map<string, string>::const_iterator it;

    it = headers.find("connection");
    if (it != headers.end() && it->second == "keep-alive") return (true);

    return (false);
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
