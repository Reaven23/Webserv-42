#include "../../includes/CGI/CGI.hpp"

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

// Constructor
CGI::CGI(Server *server, Client *client)
    : _server(server), _client(client), _errorCode(0) {
    _pipe[0] = -1;
    _pipe[1] = -1;
};

// Destructor
CGI::~CGI(){};

// Getters
int CGI::getErrorCode() const { return _errorCode; }

int *CGI::getPipe() { return (_pipe); };

const string &CGI::getScriptPath() const { return _scriptPath; }

// Public methods
bool CGI::resolvePath(const HttpRequest &request) {
    _resolvedUri.clear();

    string clean;
    int err = 0;
    if (!IHttpHandler::sanitizeUriPath(request.uri, clean, err)) {
        _errorCode = err;
        return false;
    }

    const ServerConfig   &config = _server->getConfig();
    const LocationConfig *loc = IHttpHandler::findLocation(clean, &config);

    _scriptPath =
        IHttpHandler::mapUriToFilesystemPath(clean, &config, loc);

    struct stat st;
    if (stat(_scriptPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        _errorCode = 404;
        return false;
    }
    if (access(_scriptPath.c_str(), X_OK) != 0) {
        _errorCode = 403;
        return false;
    }
    _resolvedUri = clean;
    return true;
}

bool CGI::pipe() {
    if (::pipe(_pipe) == -1) {
        Logger::error(string("setCGIResponse(): ") + strerror(errno));
        return (false);
    }

    // register pipe read-end to epoll
    epoll_event cgiEvent = {};
    cgiEvent.events = EPOLLIN;
    cgiEvent.data.fd = _pipe[0];

    if (epoll_ctl(_server->getEpollFd(), EPOLL_CTL_ADD, _pipe[0], &cgiEvent)) {
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        this->close(_pipe[0]);
        this->close(_pipe[1]);
        return (false);
    }

    // register pipe read-end to server instance
    _client->getCgiFds().push_back(_pipe[0]);

    return (true);
};

// Public method
void CGI::close(int fd) {
    // référence pour modifier le vrai vecteur du client (sans &, on modifie une copie locale)
    vector<int>&          cgis = _client->getCgiFds();
    vector<int>::iterator it = find(cgis.begin(), cgis.end(), fd);

    if (it != cgis.end()) {
        cgis.erase(it);
    }

    // :: force l'appel au close() système (<unistd.h>), sinon appel récursif infini sur CGI::close
    if (fd >= 0) ::close(fd);
}

// Private methods

vector<string> CGI::_buildEnv(const HttpRequest &request) {
    const ServerConfig &conf = _server->getConfig();
    ostringstream       portStr;
    portStr << conf.getPort();

    string methodStr;
    switch (request.method) {
        case GET:    methodStr = "GET"; break;
        case POST:   methodStr = "POST"; break;
        case DELETE: methodStr = "DELETE"; break;
        default:     methodStr = "UNKNOWN"; break;
    }

    map<string, string>::const_iterator ctIt =
        request.headers.find("content-type");
    map<string, string>::const_iterator clIt =
        request.headers.find("content-length");

    vector<string> env;
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REQUEST_METHOD=" + methodStr);
    env.push_back("SCRIPT_NAME=" + _resolvedUri);
    env.push_back("SCRIPT_FILENAME=" + _scriptPath);
    env.push_back("QUERY_STRING=" + request.queryString);
    env.push_back("SERVER_NAME=" + conf.getServerName());
    env.push_back("SERVER_PORT=" + portStr.str());
    if (ctIt != request.headers.end())
        env.push_back("CONTENT_TYPE=" + ctIt->second);
    if (clIt != request.headers.end())
        env.push_back("CONTENT_LENGTH=" + clIt->second);

    return env;
}

HttpResponse CGI::_parseOutput(const string &output) {
    HttpResponse response;

    size_t sep = output.find("\r\n\r\n");
    if (sep == string::npos) {
        return IHttpHandler::errorResponse(502, "Bad Gateway",
                                           &_server->getConfig());
    }

    string headersPart = output.substr(0, sep);
    string bodyPart = output.substr(sep + 4);

    response.setStatus(200, "OK");
    response.setBody(bodyPart);

    istringstream iss(headersPart);
    string        line;
    while (getline(iss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string key = line.substr(0, colon);
        string val = line.substr(colon + 1);
        if (!val.empty() && val[0] == ' ') val.erase(0, 1);
        response.setHeader(key, val);
    }

    response.setHeader("Connection", "close");
    return response;
}

HttpResponse CGI::_execute(const HttpRequest &request, const string &body) {
    const ServerConfig *conf = &_server->getConfig();

    if (!pipe()) {
        return IHttpHandler::errorResponse(500, "Internal Server Error", conf);
    }

    // Pipe stdin pour envoyer le body au child (POST)
    int stdinPipe[2] = {-1, -1};
    if (!body.empty()) {
        if (::pipe(stdinPipe) == -1) {
            Logger::error(string("pipe(): ") + strerror(errno));
            close(_pipe[0]);
            close(_pipe[1]);
            return IHttpHandler::errorResponse(500, "Internal Server Error", conf);
        }
    }

    int readFd = _pipe[0];
    int writeFd = _pipe[1];

    pid_t pid;
    if ((pid = fork()) == -1) {
        Logger::error(string("fork(): ") + strerror(errno));
        close(readFd);
        close(writeFd);
        if (stdinPipe[0] >= 0) { ::close(stdinPipe[0]); ::close(stdinPipe[1]); }
        return IHttpHandler::errorResponse(500, "Internal Server Error", conf);
    }

    if (pid == 0) {
        // -- child --
        close(readFd);
        if (dup2(writeFd, STDOUT_FILENO) == -1) {
            Logger::error(string("dup2(): ") + strerror(errno));
            _exit(1);
        }
        close(writeFd);

        if (stdinPipe[0] >= 0) {
            ::close(stdinPipe[1]);
            if (dup2(stdinPipe[0], STDIN_FILENO) == -1) {
                Logger::error(string("dup2(): ") + strerror(errno));
                _exit(1);
            }
            ::close(stdinPipe[0]);
        }

        vector<string> envStrs = _buildEnv(request);
        vector<char *>  envp;
        for (size_t i = 0; i < envStrs.size(); ++i)
            envp.push_back(const_cast<char *>(envStrs[i].c_str()));
        envp.push_back(NULL);

        const string &sp = _scriptPath;
        size_t        slash = sp.find_last_of('/');
        string        scriptDir;
        string        execPath;
        if (slash == string::npos) {
            scriptDir = ".";
            execPath = "./" + sp;
        } else if (slash == 0) {
            scriptDir = "/";
            execPath = "./" + sp.substr(1);
        } else {
            scriptDir = sp.substr(0, slash);
            execPath = "./" + sp.substr(slash + 1);
        }
        if (::chdir(scriptDir.c_str()) != 0) {
            Logger::error(string("chdir(): ") + strerror(errno));
            _exit(1);
        }

        char *argv[] = {const_cast<char *>(execPath.c_str()), NULL};
        execve(execPath.c_str(), argv, envp.data());
        Logger::error(string("execve(): ") + strerror(errno));
        _exit(1);
    }

    // -- parent --
    close(writeFd);

    // Envoyer le body sur stdin du child si POST
    if (stdinPipe[1] >= 0) {
        ::close(stdinPipe[0]);
        ::write(stdinPipe[1], body.c_str(), body.size());
        ::close(stdinPipe[1]);
    }

    char    buf[4096];
    string  output;
    ssize_t bytes;

    while ((bytes = ::read(readFd, buf, sizeof(buf))) > 0) {
        output.append(buf, bytes);
    }

    close(readFd);
    // 0 et pas WNOHANG : on attend la fin du child pour éviter un zombie
    waitpid(pid, NULL, 0);

    if (bytes == -1 || output.empty()) {
        return IHttpHandler::errorResponse(500, "Internal Server Error", conf);
    }

    return _parseOutput(output);
}

// Public methods

HttpResponse CGI::executeGet(const HttpRequest &request) {
    return _execute(request, "");
}

HttpResponse CGI::executePost(const HttpRequest &request) {
    return _execute(request, request.body);
}
