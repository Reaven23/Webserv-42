#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include "../../includes/CGI/CGI.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/network/Client.hpp"

using namespace std;
int CGI::getErrorCode() const { return _errorCode; }

int *CGI::getPipe() { return (_pipe); };

const string &CGI::getScriptPath() const { return _scriptPath; }

// Public methods
bool CGI::resolvePath(const HttpRequest &request) {
    _resolvedUri.clear();

    string clean;
    int    err = 0;
    if (!IHttpHandler::sanitizeUriPath(request.uri, clean, err)) {
        _errorCode = err;
        return false;
    }

    const ServerConfig   &config = _server->getConfig();
    const LocationConfig *loc = IHttpHandler::findLocation(clean, &config);

    _scriptPath = IHttpHandler::mapUriToFilesystemPath(clean, &config, loc);

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
