#pragma once

#include <sys/stat.h>

#include "./IHttpHandler.hpp"

class ServerConfig;
class LocationConfig;

class GetHttpHandler : public IHttpHandler {
  public:
    explicit GetHttpHandler(const ServerConfig* serverConfig = 0);
    virtual HttpResponse handle(const HttpRequest& request) const;

  private:
    const ServerConfig* _serverConfig;

    static HttpResponse _textResponse(int code, const std::string& reason,
                                       const std::string& body);

    static HttpResponse _redirectResponse(int code, const std::string& target);

    static std::string _contentTypeFromPath(const std::string& path);

    static std::string _resolvePath(const HttpRequest& request,
                                    const ServerConfig* serverConfig,
                                    const LocationConfig* location);

    static bool _fileExists(const std::string& path, struct stat* st);

    static bool _isReadableFile(const std::string& path);

    static HttpResponse _autoindexResponse(const std::string& dirPath,
                                            const std::string& uri);
};
