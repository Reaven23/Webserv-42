#include "../../includes/http/IHttpHandler.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"

#include <fstream>
#include <sstream>

const LocationConfig* IHttpHandler::_findLocation(
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

HttpResponse IHttpHandler::errorResponse(int code, const std::string& reason,
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

HttpResponse IHttpHandler::_methodNotAllowedResponse(
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

std::string IHttpHandler::_percentDecode(const std::string& encoded) {
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

std::string IHttpHandler::_normalizePath(const std::string& path) {
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
