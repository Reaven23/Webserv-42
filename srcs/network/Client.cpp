#include "../../includes/network/Client.hpp"

#include <sys/wait.h>

#include <iostream>
#include <sstream>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

bool Client::_locationMatchesUri(const std::string& locationPath,
                                 const std::string& uri) {
    if (locationPath.empty()) {
        return false;
    }
    if (uri.compare(0, locationPath.size(), locationPath) != 0) {
        return false;
    }
    if (locationPath == "/") {
        return true;
    }
    if (uri.size() == locationPath.size()) {
        return true;
    }
    return uri[locationPath.size()] == '/';
}

const LocationConfig* Client::_findBestLocation(const ServerConfig& config,
                                                const std::string&  uri) {
    const std::vector<LocationConfig>& locations = config.getLocations();
    const LocationConfig*              best = 0;
    size_t                             bestLen = 0;

    std::vector<LocationConfig>::const_iterator it = locations.begin();
    for (; it != locations.end(); ++it) {
        const std::string& path = it->getPath();
        if (!_locationMatchesUri(path, uri)) {
            continue;
        }
        if (path.size() > bestLen) {
            best = &(*it);
            bestLen = path.size();
        }
    }
    return best;
}

bool Client::_endsWith(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
           0;
}

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

void Client::setNotImplementedResponse() {
    _response =
        IHttpHandler::errorResponse(501, "Not Implemented", _serverConfig);
}

void Client::setCGIResponse(Server* server) {
    CGI cgi(server);

    if (!cgi.resolvePath(_request.request)) {
        int         code = cgi.getErrorCode();
        std::string reason = (code == 404) ? "Not Found" : "Forbidden";
        _response = IHttpHandler::errorResponse(code, reason, _serverConfig);
        return;
    }

    // TODO: setResponse 500;
    if (!cgi.pipe()) return;

    int readFd = cgi.getPipe()[0];
    int writeFd = cgi.getPipe()[1];

    pid_t pid;
    if ((pid = fork()) == -1) {
        Logger::error(string("fork(): ") + strerror(errno));
        cgi.close(readFd);
        cgi.close(writeFd);
        // TODO: setResponse 500;
    };

    if (pid == 0) {
        cgi.close(readFd);
        if (dup2(writeFd, STDOUT_FILENO) == -1) {
            Logger::error(string("dup2(): ") + strerror(errno));
            cgi.close(writeFd);
            // TODO: setResponse 500;
            // TODO: quel code de sortie ?;
            exit(1);
        };
        cgi.close(writeFd);

        // Build CGI environment variables
        const ServerConfig &conf = server->getConfig();
        ostringstream       portStr;
        portStr << conf.getPort();

        string methodStr;
        switch (_request.request.method) {
            case GET:    methodStr = "GET"; break;
            case POST:   methodStr = "POST"; break;
            case DELETE: methodStr = "DELETE"; break;
            default:     methodStr = "UNKNOWN"; break;
        }

        map<string, string>::const_iterator ctIt =
            _request.request.headers.find("content-type");
        map<string, string>::const_iterator clIt =
            _request.request.headers.find("content-length");

        vector<string> envStrs;
        envStrs.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envStrs.push_back("SERVER_PROTOCOL=HTTP/1.1");
        envStrs.push_back("REQUEST_METHOD=" + methodStr);
        envStrs.push_back("SCRIPT_NAME=" + _request.request.uri);
        envStrs.push_back("SCRIPT_FILENAME=" + cgi.getScriptPath());
        envStrs.push_back("QUERY_STRING=" + _request.request.queryString);
        envStrs.push_back("SERVER_NAME=" + conf.getServerName());
        envStrs.push_back("SERVER_PORT=" + portStr.str());
        envStrs.push_back("REDIRECT_STATUS=200");
        if (ctIt != _request.request.headers.end())
            envStrs.push_back("CONTENT_TYPE=" + ctIt->second);
        if (clIt != _request.request.headers.end())
            envStrs.push_back("CONTENT_LENGTH=" + clIt->second);

        vector<char *> envp;
        for (size_t i = 0; i < envStrs.size(); ++i)
            envp.push_back(const_cast<char *>(envStrs[i].c_str()));
        envp.push_back(NULL);

        // TODO: execve();
    } else {
        cgi.close(writeFd);

        char    buffer[4096];
        string  output;
        ssize_t bytes;

        while ((bytes = ::read(readFd, buffer, sizeof(buffer)) > 0)) {
            output.append(buffer, bytes);
        }

        if (bytes == -1) {
            Logger::error(string("read(): ") + strerror(errno));
            // TODO: setResponse 500;
        }

        cgi.close(readFd);
        waitpid(pid, NULL, WNOHANG);
    }
    setLastActivity();
    setResponse();
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

bool Client::isCGIRequest(Server* server) const {
    if (server == 0 || _request.status != OK || _request.request.uri.empty())
        return false;
    const LocationConfig* location =
        _findBestLocation(server->getConfig(), _request.request.uri);
    if (location == 0 || !location->hasCgiExtension()) return false;
    return _endsWith(_request.request.uri, location->getCgiExtension());
}

bool Client::isSupportedCgi(Server* server) const {
    if (server == 0) return false;
    const LocationConfig* location =
        _findBestLocation(server->getConfig(), _request.request.uri);
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
