#include "../../includes/http/DeleteHttpHandler.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <sstream>

#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"

using namespace std;

DeleteHttpHandler::DeleteHttpHandler(const ServerConfig* serverConfig)
    : _serverConfig(serverConfig) {}

std::string DeleteHttpHandler::_resolvePath(const HttpRequest&    request,
                                            const ServerConfig*   serverConfig,
                                            const LocationConfig* location) {
    return mapUriToFilesystemPath(request.uri, serverConfig, location);
}

HttpResponse DeleteHttpHandler::handle(const HttpRequest& request) const {
    std::string cleanUri;
    int         uriErr = 0;
    if (!sanitizeUriPath(request.uri, cleanUri, uriErr))
        return errorResponse(uriErr == 400 ? 400 : 403,
                             uriErr == 400 ? "Bad Request" : "Forbidden",
                             _serverConfig);

    HttpRequest sanitized = request;
    sanitized.uri = cleanUri;

    const LocationConfig* location = findLocation(sanitized.uri, _serverConfig);

    // --- Methods check ---
    if (location != NULL) {
        const vector<string>& methods = location->getMethods();
        if (!methods.empty()) {
            bool allowed = false;
            for (size_t i = 0; i < methods.size(); ++i) {
                if (methods[i] == "DELETE") {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) return _methodNotAllowedResponse(methods);
        }
    }

    // --- Resolve filesystem path ---
    string path = _resolvePath(sanitized, _serverConfig, location);

    // --- Check file exists ---
    struct stat st = {};
    if (stat(path.c_str(), &st) != 0)
        return errorResponse(404, "Not Found", _serverConfig);

    // --- Refuse to delete directories ---
    if (S_ISDIR(st.st_mode))
        return errorResponse(403, "Forbidden", _serverConfig);

    // --- Must be a regular file ---
    if (!S_ISREG(st.st_mode))
        return errorResponse(403, "Forbidden", _serverConfig);

    // --- Check write permission ---
    if (access(path.c_str(), W_OK) != 0)
        return errorResponse(403, "Forbidden", _serverConfig);

    // --- Delete the file ---
    if (std::remove(path.c_str()) != 0)
        return errorResponse(500, "Internal Server Error", _serverConfig);

    // --- Success ---
    HttpResponse response(200, "OK");

    ostringstream bodyMsg;
    bodyMsg << "File deleted: " << sanitized.uri;
    response.setBody(bodyMsg.str());
    response.setHeader("Content-Type", "text/plain");

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    return response;
}
