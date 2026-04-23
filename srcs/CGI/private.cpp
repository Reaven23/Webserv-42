#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/network/Client.hpp"
#include "../../includes/utils/Logger.hpp"

using namespace std;

vector<string> CGI::_buildEnv(const HttpRequest &request) {
    const ServerConfig &conf = _server->getConfig();
    ostringstream       portStr;
    portStr << conf.getPort();

    string methodStr;
    switch (request.method) {
        case GET:
            methodStr = "GET";
            break;
        case POST:
            methodStr = "POST";
            break;
        case DELETE:
            methodStr = "DELETE";
            break;
        default:
            methodStr = "UNKNOWN";
            break;
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
    int    skip = 4;
    if (sep == string::npos) {
        sep = output.find("\n\n");
        if (sep == string::npos) {
            // No headers, output is the body
            ostringstream os;
            os << output.size();
            response.setStatus(200, "OK")
                .setHeader("Content-Type", "text/plain")
                .setHeader("Content-Length", os.str())
                .setBody(output);

            return response;
        }
        skip = 2;
    }

    string headersPart = output.substr(0, sep);
    string bodyPart = output.substr(sep + skip);

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

    if (response.headers.find("Content-Length") == response.headers.end()) {
        ostringstream os;
        os << response.body.size();
        response.setHeader("Content-Length", os.str());
    }

    return response;
}

void CGI::_handleWaitingState(Client *client, ServerConfig const *serverConfig,
                              HttpMethod const method) {
    pid_t pid;
    if ((pid = fork()) == -1) {
        Logger::error(string("fork(): ") + strerror(errno));

        client->getCgis().erase(_pipe[0]);
        close(_pipe[0]);
        close(_pipe[1]);
        _pipe[0] = -1;
        _pipe[1] = -1;
        if (method == POST) {
            client->getCgis().erase(_stdinPipe[1]);
            close(_stdinPipe[0]);
            close(_stdinPipe[1]);
            _stdinPipe[0] = -1;
            _stdinPipe[1] = -1;
        }

        client->setErrorResponse(500, serverConfig);
        client->setState(Client::SENDING_RESPONSE);
        client->switchToEpollOut();
        return;
    }

    // -- child --
    if (pid == 0) {
        close(_pipe[0]);

        if (method == POST) {
            close(_stdinPipe[1]);
            if (dup2(_stdinPipe[0], STDIN_FILENO) == -1) {
                close(_pipe[1]);
                close(_stdinPipe[0]);
                Logger::error(string("dup2(): ") + strerror(errno));
                _exit(1);
            }
            close(_stdinPipe[0]);
        }

        if (dup2(_pipe[1], STDOUT_FILENO) == -1) {
            close(_pipe[1]);
            Logger::error(string("dup2(): ") + strerror(errno));
            _exit(1);
        }
        close(_pipe[1]);

        vector<string> envStrs = _buildEnv(client->getRequest().request);
        vector<char *> envp;
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
    } else {
        // -- parent --
        close(_pipe[1]);
        _pipe[1] = -1;
        if (method == POST) {
            close(_stdinPipe[0]);
            _stdinPipe[0] = -1;
        }

        _childPid = pid;
        method == GET ? client->setState(Client::READING_CGI)
                      : client->setState(Client::WRITING_CGI);
        return;
    }
};

void CGI::_handleWritingState(Client             *client,
                              ServerConfig const *serverConfig) {
    (void)serverConfig;
    ssize_t       bytes = 0;
    string const &body = client->getRequest().request.body;

    bytes = write(_stdinPipe[1], body.c_str() + _bodyOffset,
                  body.size() - _bodyOffset);

    if (bytes < 0) return;

    _bodyOffset += bytes;

    if (_bodyOffset >= body.size()) {
        client->setState(Client::READING_CGI);
        client->getCgis().erase(_stdinPipe[1]);
        close(_stdinPipe[1]);
        _stdinPipe[1] = -1;
    }
};

void CGI::_handleReadingState(Client             *client,
                              ServerConfig const *serverConfig) {
    char    buf[4096] = {0};
    string  output;
    ssize_t bytes;

    bytes = ::read(_pipe[0], buf, sizeof(buf));
    if (bytes > 0) {
        client->appendToCGIBuffer(buf, bytes);
    } else if (bytes == 0) {
        client->getCgis().erase(_pipe[0]);
        close(_pipe[0]);
        _pipe[0] = -1;

        int   status = 0;
        pid_t reaped = waitpid(_childPid, &status, WNOHANG);
        bool  failed = (reaped > 0) && ((WIFSIGNALED(status)) ||
                                       (WIFEXITED(status) &&
                                        WEXITSTATUS(status) != 0));
        if (reaped > 0) _childPid = -1;

        if (failed) {
            client->setErrorResponse(500, serverConfig);
        } else {
            HttpResponse resp = _parseOutput(client->getCgiBuffer());
            client->getResponse() = resp;
        }

        client->setState(Client::SENDING_RESPONSE);
        client->switchToEpollOut();
    }
};
