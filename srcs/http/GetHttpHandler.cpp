#include "../../includes/http/GetHttpHandler.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cctype>
#include <fstream>
#include <sstream>

#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"

using namespace std;

GetHttpHandler::GetHttpHandler(const ServerConfig* serverConfig)
    : _serverConfig(serverConfig) {}

HttpResponse GetHttpHandler::_textResponse(int code, const string& reason,
                                           const string& body) {
    HttpResponse response(code, reason);
    response.setBody(body);
    response.setHeader("Content-Type", "text/plain");

    ostringstream contentLength;
    contentLength << response.body.size();
    response.setHeader("Content-Length", contentLength.str());
    return response;
}

string GetHttpHandler::_extensionLower(const string& path) {
    string::size_type dot = path.find_last_of('.');
    if (dot == string::npos || dot + 1 >= path.size()) return "";
    string ext = path.substr(dot);
    for (size_t i = 0; i < ext.size(); ++i) {
        ext[i] = static_cast<char>(tolower(static_cast<unsigned char>(ext[i])));
    }
    return ext;
}

string GetHttpHandler::_contentTypeFromPath(const string& path) {
    string ext = _extensionLower(path);
    if (ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    return "application/octet-stream";
}

HttpResponse GetHttpHandler::_redirectResponse(int code, const string& target) {
    string reason;
    if (code == 301)
        reason = "Moved Permanently";
    else if (code == 302)
        reason = "Found";
    else if (code == 307)
        reason = "Temporary Redirect";
    else if (code == 308)
        reason = "Permanent Redirect";
    else
        reason = "Found";

    HttpResponse response(code, reason);
    response.setHeader("Location", target);
    response.setBody("<html><body><a href=\"" + target + "\">" + reason +
                     "</a></body></html>");
    response.setHeader("Content-Type", "text/html");

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    return response;
}

bool GetHttpHandler::_fileExists(const string& path, struct stat* st) {
    return stat(path.c_str(), st) == 0;
}

bool GetHttpHandler::_isReadableFile(const string& path) {
    return access(path.c_str(), R_OK) == 0;
}

HttpResponse GetHttpHandler::_autoindexResponse(const string& dirPath,
                                                const string& uri) {
    DIR* dir = opendir(dirPath.c_str());
    if (dir == NULL) return errorResponse(500, "Internal Server Error", NULL);

    string uriSlash = uri;
    if (uriSlash.empty() || uriSlash[uriSlash.size() - 1] != '/')
        uriSlash += "/";

    ostringstream html;
    html << "<html>\r\n<head><title>Index of " << uriSlash
         << "</title></head>\r\n<body>\r\n<h1>Index of " << uriSlash
         << "</h1><hr>\r\n<pre>\r\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (name == ".") continue;

        string      fullPath = dirPath + "/" + name;
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

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    return response;
}

string GetHttpHandler::_resolvePath(const HttpRequest&    request,
                                    const ServerConfig*   serverConfig,
                                    const LocationConfig* location) {
    string uriPath = request.uri;

    std::string baseRoot = ".";
    std::string baseIndex = "index.html";

    if (serverConfig != NULL) {
        if (!serverConfig->getRoot().empty())
            baseRoot = serverConfig->getRoot();
        if (!serverConfig->getIndex().empty())
            baseIndex = serverConfig->getIndex();
    }

    if (location != NULL) {
        if (!location->getRoot().empty()) baseRoot = location->getRoot();
        if (!location->getIndex().empty()) baseIndex = location->getIndex();
    }

    if (uriPath == "/") return baseRoot + "/" + baseIndex;

    std::string direct =
        mapUriToFilesystemPath(uriPath, serverConfig, location);
    struct stat st = {};
    if (_fileExists(direct, &st) && S_ISREG(st.st_mode)) return direct;

    string indexCandidate = direct;
    if (!indexCandidate.empty() &&
        indexCandidate[indexCandidate.size() - 1] != '/')
        indexCandidate += "/";
    indexCandidate += baseIndex;
    if (_fileExists(indexCandidate, &st) && S_ISREG(st.st_mode))
        return indexCandidate;

    string htmlCandidate = direct + ".html";
    if (_fileExists(htmlCandidate, &st) && S_ISREG(st.st_mode))
        return htmlCandidate;

    return direct;
}

HttpResponse GetHttpHandler::handle(const HttpRequest& request) const {
    std::string cleanUri;
    int         uriErr = 0;
    if (!sanitizeUriPath(request.uri, cleanUri, uriErr))
        return errorResponse(uriErr == 400 ? 400 : 403,
                             uriErr == 400 ? "Bad Request" : "Forbidden",
                             _serverConfig);

    HttpRequest sanitized = request;
    sanitized.uri = cleanUri;

    const LocationConfig* location = findLocation(sanitized.uri, _serverConfig);

    if (location != NULL) {
        const vector<string>& methods = location->getMethods();
        if (!methods.empty()) {
            bool allowed = false;
            for (size_t i = 0; i < methods.size(); ++i) {
                if (methods[i] == "GET") {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) return _methodNotAllowedResponse(methods);
        }

        if (location->hasRedirect())
            return _redirectResponse(location->getRedirectCode(),
                                     location->getRedirectTarget());
    }

    string path = _resolvePath(sanitized, _serverConfig, location);

    struct stat st = {};
    if (!_fileExists(path, &st))
        return errorResponse(404, "Not Found", _serverConfig);
    if (S_ISDIR(st.st_mode)) {
        if (location != NULL &&
            location->getAutoindexState() == LocationConfig::AUTOINDEX_ON)
            return _autoindexResponse(path, sanitized.uri);
        return errorResponse(403, "Forbidden", _serverConfig);
    }
    if (!S_ISREG(st.st_mode))
        return errorResponse(403, "Forbidden", _serverConfig);
    if (!_isReadableFile(path))
        return errorResponse(403, "Forbidden", _serverConfig);

    ifstream file(path.c_str(), ios::in | ios::binary);
    if (!file.is_open())
        return errorResponse(500, "Internal Server Error", _serverConfig);

    ostringstream body;
    body << file.rdbuf();
    if (!file.good() && !file.eof())
        return errorResponse(500, "Internal Server Error", _serverConfig);

    HttpResponse response(200, "OK");
    response.setBody(body.str());
    response.setHeader("Content-Type", _contentTypeFromPath(path));

    ostringstream contentLength;
    contentLength << response.body.size();
    response.setHeader("Content-Length", contentLength.str());
    return response;
}
