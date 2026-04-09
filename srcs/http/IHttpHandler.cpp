#include "../../includes/http/IHttpHandler.hpp"

#include <fstream>
#include <sstream>

#include "../../includes/config/ServerConfig.hpp"

using namespace std;

const LocationConfig* IHttpHandler::findLocation(
    const std::string& uri, const ServerConfig* serverConfig) {
    if (serverConfig == NULL) return NULL;

    const LocationConfig*         best = NULL;
    const vector<LocationConfig>& locations = serverConfig->getLocations();
    size_t                        bestLen = 0;
    for (vector<LocationConfig>::const_iterator it = locations.begin();
         it != locations.end(); ++it) {
        const string& locPath = it->getPath();
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

std::string IHttpHandler::mapUriToFilesystemPath(
    const std::string& sanitizedUri, const ServerConfig* serverConfig,
    const LocationConfig* location) {
    std::string baseRoot = ".";
    if (serverConfig != NULL && !serverConfig->getRoot().empty())
        baseRoot = serverConfig->getRoot();

    std::string relPath = sanitizedUri;
    if (location != NULL) {
        if (!location->getRoot().empty()) baseRoot = location->getRoot();
        relPath = sanitizedUri.substr(location->getPath().size());
        if (relPath.empty() || relPath[0] != '/') relPath = "/" + relPath;
    }
    return baseRoot + relPath;
}

bool IHttpHandler::sanitizeUriPath(const std::string& uri,
                                   std::string& outClean, int& errorCode) {
    if (uri.empty() || uri[0] != '/') {
        errorCode = 400;
        return false;
    }
    std::string decoded = _percentDecode(uri);
    if (decoded.empty()) {
        errorCode = 400;
        return false;
    }
    std::string cleanUri = _normalizePath(decoded);
    if (cleanUri.empty()) {
        errorCode = 403;
        return false;
    }
    outClean = cleanUri;
    return true;
}

HttpResponse IHttpHandler::errorResponse(int code, const std::string& reason,
                                         const ServerConfig* serverConfig) {
    if (serverConfig != NULL) {
        const map<int, string>& errorPages = serverConfig->getErrorPages();
        map<int, string>::const_iterator it = errorPages.find(code);
        if (it != errorPages.end()) {
            ifstream file(it->second.c_str(), ios::in | ios::binary);
            if (file.is_open()) {
                ostringstream body;
                body << file.rdbuf();
                if (file.good() || file.eof()) {
                    HttpResponse response(code, reason);
                    response.setBody(body.str());
                    response.setHeader("Content-Type", "text/html");
                    ostringstream cl;
                    cl << response.body.size();
                    response.setHeader("Content-Length", cl.str());
                    response.setHeader("Connection", "close");
                    return response;
                }
            }
        }
    }

    ostringstream html;
    html << "<html>\r\n<head><title>" << code << " " << reason
         << "</title></head>\r\n<body>\r\n<center><h1>" << code << " " << reason
         << "</h1></center>\r\n<hr><center>webserv</center>\r\n"
         << "</body>\r\n</html>\r\n";

    HttpResponse response(code, reason);
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

HttpResponse IHttpHandler::_methodNotAllowedResponse(
    const vector<string>& allowedMethods) {
    HttpResponse response(405, "Method Not Allowed");
    response.setBody("Method Not Allowed");
    response.setHeader("Content-Type", "text/plain");

    string allow;
    for (size_t i = 0; i < allowedMethods.size(); ++i) {
        if (i > 0) allow += ", ";
        allow += allowedMethods[i];
    }
    response.setHeader("Allow", allow);

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}

string IHttpHandler::_percentDecode(const string& encoded) {
    string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            char hi = encoded[i + 1];
            char lo = encoded[i + 2];
            int  a = -1, b = -1;
            if (hi >= '0' && hi <= '9')
                a = hi - '0';
            else if (hi >= 'a' && hi <= 'f')
                a = hi - 'a' + 10;
            else if (hi >= 'A' && hi <= 'F')
                a = hi - 'A' + 10;
            if (lo >= '0' && lo <= '9')
                b = lo - '0';
            else if (lo >= 'a' && lo <= 'f')
                b = lo - 'a' + 10;
            else if (lo >= 'A' && lo <= 'F')
                b = lo - 'A' + 10;
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

string IHttpHandler::_normalizePath(const string& path) {
    if (path.empty() || path[0] != '/') return "";

    vector<string> segments;
    istringstream  ss(path.substr(1));
    string         segment;

    while (getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".") continue;
        if (segment == "..") {
            if (segments.empty()) return "";
            segments.pop_back();
        } else {
            segments.push_back(segment);
        }
    }

    string result = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) result += "/";
        result += segments[i];
    }
    return result;
}
