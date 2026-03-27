#pragma once

#include <sys/stat.h>
#include <vector>

#include "./IHttpHandler.hpp"

class ServerConfig;
class LocationConfig;

class GetHttpHandler : public IHttpHandler {
  public:
    explicit GetHttpHandler(const ServerConfig* serverConfig = 0);
    virtual HttpResponse handle(const HttpRequest& request) const;

  private:
    const ServerConfig* _serverConfig;

    static const LocationConfig* _findLocation(const std::string& uri,
                                               const ServerConfig* serverConfig);

    static HttpResponse _textResponse(int code, const std::string& reason,
                                       const std::string& body);

    static HttpResponse _errorResponse(int code, const std::string& reason,
                                        const ServerConfig* serverConfig);

    static HttpResponse _methodNotAllowedResponse(
        const std::vector<std::string>& allowedMethods);

    static HttpResponse _redirectResponse(int code, const std::string& target);

    static std::string _contentTypeFromPath(const std::string& path);

    static std::string _resolvePath(const HttpRequest& request,
                                    const ServerConfig* serverConfig,
                                    const LocationConfig* location);

    static bool _fileExists(const std::string& path, struct stat* st);

    static bool _isReadableFile(const std::string& path);

    static HttpResponse _autoindexResponse(const std::string& dirPath,
                                            const std::string& uri);

    static std::string _percentDecode(const std::string& encoded);
    static std::string _normalizePath(const std::string& path);
};
