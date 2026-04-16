#pragma once

#include <string>
#include <vector>

#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"

class ServerConfig;
class LocationConfig;

class IHttpHandler {
   public:
    virtual ~IHttpHandler() {}
    virtual HttpResponse handle(const HttpRequest& request) const = 0;

    static HttpResponse errorResponse(int code, const std::string& reason,
                                       const ServerConfig* serverConfig);

    // Subject IV.3: route /kapouet → dir /tmp/www ⇒ /kapouet/pouic/... → /tmp/www/pouic/...
    static std::string mapUriToFilesystemPath(const std::string& sanitizedUri,
                                              const ServerConfig* serverConfig,
                                              const LocationConfig* location);

    static const LocationConfig* findLocation(const std::string& uri,
                                              const ServerConfig* serverConfig);

    static bool sanitizeUriPath(const std::string& uri, std::string& outClean,
                                int& errorCode);

   protected:
    static HttpResponse _methodNotAllowedResponse(
        const std::vector<std::string>& allowedMethods);

    static std::string _percentDecode(const std::string& encoded);
    static std::string _normalizePath(const std::string& path);
};
