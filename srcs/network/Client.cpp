#include "../../includes/network/Client.hpp"

#include <sys/types.h>

#include <iostream>
#include <sstream>

#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

// Constructors
Client::Client(const ServerConfig* serverConfig)
    : _fd(-1), _lastActivity(time(NULL)), _serverConfig(serverConfig){};

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
    _response.setHeader("Connection", "close");
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

void Client::setCGIResponse(Server* server) {
    //  cgi.pipe
    //  cgi.register
    //  fork
    //  si process enfant: close read end du pipe + dup2 + CgiHandleRequest +
    //  execve avec F_CLOSE
    //  si parent: close write end du pipe + wait pid
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

    // Temp buffer to store the received data.
    char    tmp[1024] = {0};
    ssize_t bytes = recv(_fd, tmp, sizeof(tmp), 0);

    // recv() reads data sent by the client through the connection.
    //   - clientSocket   : the client fd (returned by accept, NOT the listening
    //   socket!)
    //   - buffer         : where to store the received bytes
    //   - sizeof(buffer) : max number of bytes to read (to avoid buffer
    //   overflow)
    //   - 0              : no flags (default behavior, blocking)
    // Returns the number of bytes read, 0 if the client disconnected, -1 on
    // error
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
    string        raw = _response.toString();

    if (raw.empty()) {
        os << "send(): Failed to send data to client '" << _fd << "|" << getIp()
           << "'" << endl;
        os << strerror(errno);

        Logger::error(os.str());

        return (-1);
    }

    ssize_t bytes = ::send(_fd, raw.c_str(), raw.size(), 0);

    if (bytes == -1) {
        os << "send(): Failed to send data to client '" << _fd << "|" << getIp()
           << "'" << endl;
        os << strerror(errno);

        Logger::error(os.str());

        return (-1);
    }

    setLastActivity();

    return (bytes);
}

void Client::reset() {
    _buffer.erase(0, _request.consumed);

    _request.consumed = 0;
    _request.status = INCOMPLETE;
    _request.request = HttpRequest();

    _response = HttpResponse();
};

bool Client::isRequestComplete() const { return (_request.status == OK); };

bool Client::isRequestError() const { return (_request.status == ERROR); };

bool Client::isResponseComplete() const { return (true); };

bool Client::isKeepAlive() const {
    map<string, string>                 headers = _request.request.headers;
    map<string, string>::const_iterator it;

    it = headers.find("connection");
    if (it != headers.end()) return (true);

    return (false);
}

bool Client::isCGIRequest() const { return (_request.cgi); }

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
