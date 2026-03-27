#include "../../includes/http/GetHttpHandler.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"


#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

GetHttpHandler::GetHttpHandler(const ServerConfig* serverConfig)
    : _serverConfig(serverConfig) {}

HttpResponse GetHttpHandler::_textResponse(int code, const std::string& reason,
                                          const std::string& body) {
    HttpResponse response(code, reason);
    response.setBody(body);
    response.setHeader("Content-Type", "text/plain");

    std::ostringstream contentLength;
    contentLength << response.body.size();
    response.setHeader("Content-Length", contentLength.str());
    response.setHeader("Connection", "close");
    return response;
}

HttpResponse GetHttpHandler::_errorResponse(int code, const std::string& reason,
                                             const ServerConfig* serverConfig) {
    if (serverConfig != NULL) {
        const std::map<int, std::string>& errorPages =
            serverConfig->getErrorPages();
        std::map<int, std::string>::const_iterator it = errorPages.find(code);
        if (it != errorPages.end()) {
            std::ifstream file(it->second.c_str(),
                               std::ios::in | std::ios::binary);
            if (file.is_open()) {
                std::ostringstream body;
                body << file.rdbuf();
                if (file.good() || file.eof()) {
                    HttpResponse response(code, reason);
                    response.setBody(body.str());
                    response.setHeader("Content-Type", "text/html");
                    std::ostringstream cl;
                    cl << response.body.size();
                    response.setHeader("Content-Length", cl.str());
                    response.setHeader("Connection", "close");
                    return response;
                }
            }
        }
    }

    std::ostringstream html;
    html << "<html>\r\n<head><title>" << code << " " << reason
         << "</title></head>\r\n<body>\r\n<center><h1>" << code << " " << reason
         << "</h1></center>\r\n<hr><center>webserv</center>\r\n"
         << "</body>\r\n</html>\r\n";

    HttpResponse response(code, reason);
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");

    std::ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

std::string GetHttpHandler::_contentTypeFromPath(const std::string& path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html")
        return "text/html";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")
        return "text/css";
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
        return "application/javascript";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".svg")
        return "image/svg+xml";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")
        return "image/png";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".gif")
        return "image/gif";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".webp")
        return "image/webp";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".ico")
        return "image/x-icon";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")
        return "image/jpeg";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".jpeg")
        return "image/jpeg";
    return "application/octet-stream";
}

std::string GetHttpHandler::_percentDecode(const std::string& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            char hi = encoded[i + 1];
            char lo = encoded[i + 2];
            int a = -1, b = -1;
            if (hi >= '0' && hi <= '9')      a = hi - '0';
            else if (hi >= 'a' && hi <= 'f') a = hi - 'a' + 10;
            else if (hi >= 'A' && hi <= 'F') a = hi - 'A' + 10;
            if (lo >= '0' && lo <= '9')      b = lo - '0';
            else if (lo >= 'a' && lo <= 'f') b = lo - 'a' + 10;
            else if (lo >= 'A' && lo <= 'F') b = lo - 'A' + 10;
            if (a >= 0 && b >= 0) {
                char c = static_cast<char>((a << 4) | b);
                if (c == '\0') return "";
                result += c;
                i += 2;
                continue;
            }
        }
        result += encoded[i];
    }
    return result;
}

std::string GetHttpHandler::_normalizePath(const std::string& path) {
    if (path.empty() || path[0] != '/')
        return "";

    std::vector<std::string> segments;
    std::istringstream ss(path.substr(1));
    std::string segment;

    while (std::getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".")
            continue;
        if (segment == "..") {
            if (segments.empty())
                return "";
            segments.pop_back();
        } else {
            segments.push_back(segment);
        }
    }

    std::string result = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) result += "/";
        result += segments[i];
    }
    return result;
}

const LocationConfig* GetHttpHandler::_findLocation(
    const std::string& uri, const ServerConfig* serverConfig) {
    if (serverConfig == NULL)
        return NULL;

    const LocationConfig* best = NULL;
    const std::vector<LocationConfig>& locations = serverConfig->getLocations();
    size_t bestLen = 0;
    for (std::vector<LocationConfig>::const_iterator it = locations.begin();
         it != locations.end(); ++it) {
        const std::string& locPath = it->getPath();
        if (locPath.empty()) continue;
        if (uri.compare(0, locPath.size(), locPath) != 0) continue;
        if (uri.size() > locPath.size() && locPath[locPath.size() - 1] != '/' &&
            uri[locPath.size()] != '/')
            continue;
        if (best == NULL || locPath.size() > bestLen) {
            best = &(*it);
            bestLen = locPath.size();
        }
    }
    return best;
}

HttpResponse GetHttpHandler::_methodNotAllowedResponse(
    const std::vector<std::string>& allowedMethods) {
    HttpResponse response(405, "Method Not Allowed");
    response.setBody("Method Not Allowed");
    response.setHeader("Content-Type", "text/plain");

    std::string allow;
    for (size_t i = 0; i < allowedMethods.size(); ++i) {
        if (i > 0) allow += ", ";
        allow += allowedMethods[i];
    }
    response.setHeader("Allow", allow);

    std::ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

HttpResponse GetHttpHandler::_redirectResponse(int code,
                                               const std::string& target) {
    std::string reason;
    if (code == 301)      reason = "Moved Permanently";
    else if (code == 302) reason = "Found";
    else if (code == 307) reason = "Temporary Redirect";
    else if (code == 308) reason = "Permanent Redirect";
    else                  reason = "Found";

    HttpResponse response(code, reason);
    response.setHeader("Location", target);
    response.setBody("<html><body><a href=\"" + target + "\">" + reason +
                     "</a></body></html>");
    response.setHeader("Content-Type", "text/html");

    std::ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

bool GetHttpHandler::_fileExists(const std::string& path, struct stat* st) {
    return stat(path.c_str(), st) == 0;
}

bool GetHttpHandler::_isReadableFile(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}

HttpResponse GetHttpHandler::_autoindexResponse(const std::string& dirPath,
                                                  const std::string& uri) {
    DIR* dir = opendir(dirPath.c_str());
    if (dir == NULL)
        return _errorResponse(500, "Internal Server Error", NULL);

    std::string uriSlash = uri;
    if (uriSlash.empty() || uriSlash[uriSlash.size() - 1] != '/')
        uriSlash += "/";

    std::ostringstream html;
    html << "<html>\r\n<head><title>Index of " << uriSlash
         << "</title></head>\r\n<body>\r\n<h1>Index of " << uriSlash
         << "</h1><hr>\r\n<pre>\r\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".")
            continue;

        std::string fullPath = dirPath + "/" + name;
        struct stat st = {};
        bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

        html << "<a href=\"" << uriSlash << name;
        if (isDir) html << "/";
        html << "\">" << name;
        if (isDir) html << "/";
        html << "</a>\r\n";
    }
    closedir(dir);

    html << "</pre>\r\n<hr>\r\n</body>\r\n</html>\r\n";

    HttpResponse response(200, "OK");
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");

    std::ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

std::string GetHttpHandler::_resolvePath(const HttpRequest& request,
                                         const ServerConfig* serverConfig,
                                         const LocationConfig* location) {
    std::string uriPath = request.uri;

    std::string baseRoot = ".";
    std::string baseIndex = "index.html";
    std::string relPath = uriPath;

    if (serverConfig != NULL) {
        if (!serverConfig->getRoot().empty()) baseRoot = serverConfig->getRoot();
        if (!serverConfig->getIndex().empty())
            baseIndex = serverConfig->getIndex();
    }

    if (location != NULL) {
        if (!location->getRoot().empty()) baseRoot = location->getRoot();
        if (!location->getIndex().empty()) baseIndex = location->getIndex();
        relPath = uriPath.substr(location->getPath().size());
        if (relPath.empty() || relPath[0] != '/') relPath = "/" + relPath;
    }

    if (uriPath == "/") return baseRoot + "/" + baseIndex;

    std::string direct = baseRoot + relPath;
    struct stat st = {};
    if (_fileExists(direct, &st) && S_ISREG(st.st_mode))
        return direct;

    std::string indexCandidate = direct;
    if (!indexCandidate.empty() &&
        indexCandidate[indexCandidate.size() - 1] != '/')
        indexCandidate += "/";
    indexCandidate += baseIndex;
    if (_fileExists(indexCandidate, &st) && S_ISREG(st.st_mode))
        return indexCandidate;

    std::string htmlCandidate = direct + ".html";
    if (_fileExists(htmlCandidate, &st) && S_ISREG(st.st_mode))
        return htmlCandidate;

    return direct;
}

HttpResponse GetHttpHandler::handle(const HttpRequest& request) const {
    if (request.uri.empty() || request.uri[0] != '/')
        return _errorResponse(400, "Bad Request", _serverConfig);

    std::string decoded = _percentDecode(request.uri);
    if (decoded.empty())
        return _errorResponse(400, "Bad Request", _serverConfig);

    std::string cleanUri = _normalizePath(decoded);
    if (cleanUri.empty())
        return _errorResponse(403, "Forbidden", _serverConfig);

    HttpRequest sanitized = request;
    sanitized.uri = cleanUri;

    const LocationConfig* location = _findLocation(sanitized.uri, _serverConfig);

    if (location != NULL) {
        const std::vector<std::string>& methods = location->getMethods();
        if (!methods.empty()) {
            bool allowed = false;
            for (size_t i = 0; i < methods.size(); ++i) {
                if (methods[i] == "GET") {
                    allowed = true;
                    break;
                }
            }
            if (!allowed)
                return _methodNotAllowedResponse(methods);
        }

        if (location->hasRedirect())
            return _redirectResponse(location->getRedirectCode(),
                                     location->getRedirectTarget());
    }

    std::string path = _resolvePath(sanitized, _serverConfig, location);

    struct stat st = {};
    if (!_fileExists(path, &st))
        return _errorResponse(404, "Not Found", _serverConfig);
    if (S_ISDIR(st.st_mode)) {
        if (location != NULL &&
            location->getAutoindexState() == LocationConfig::AUTOINDEX_ON)
            return _autoindexResponse(path, sanitized.uri);
        return _errorResponse(403, "Forbidden", _serverConfig);
    }
    if (!S_ISREG(st.st_mode))
        return _errorResponse(403, "Forbidden", _serverConfig);
    if (!_isReadableFile(path))
        return _errorResponse(403, "Forbidden", _serverConfig);

    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return _errorResponse(500, "Internal Server Error", _serverConfig);

    std::ostringstream body;
    body << file.rdbuf();
    if (!file.good() && !file.eof())
        return _errorResponse(500, "Internal Server Error", _serverConfig);

    HttpResponse response(200, "OK");
    response.setBody(body.str());
    response.setHeader("Content-Type", _contentTypeFromPath(path));

    std::ostringstream contentLength;
    contentLength << response.body.size();
    response.setHeader("Content-Length", contentLength.str());
    response.setHeader("Connection", "close");
    return response;
}
