#pragma once

#include <vector>

#include "./IHttpHandler.hpp"

class ServerConfig;
class LocationConfig;

class PostHttpHandler : public IHttpHandler {
  public:
    PostHttpHandler(const ServerConfig* serverConfig = 0);
    virtual HttpResponse handle(const HttpRequest& request) const;

  private:
    const ServerConfig* _serverConfig;

    struct UploadedFile {
        std::string filename;
        std::string contentType;
        std::string data;
    };

    static bool        _isMultipart(const HttpRequest& request, std::string& boundary);
    static std::string _extractBoundary(const std::string& contentType);
    static std::vector<UploadedFile> _parseMultipart(const std::string& body,
                                                      const std::string& boundary);
    static std::string _sanitizeFilename(const std::string& raw);
};
