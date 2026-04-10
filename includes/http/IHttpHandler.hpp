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

   protected:
    static const LocationConfig* _findLocation(const std::string& uri,
                                               const ServerConfig* serverConfig);

    static HttpResponse _methodNotAllowedResponse(
        const std::vector<std::string>& allowedMethods);

    static std::string _percentDecode(const std::string& encoded);
    static std::string _normalizePath(const std::string& path);
};
