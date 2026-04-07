#pragma once

#include "./IHttpHandler.hpp"

class ServerConfig;
class LocationConfig;

class DeleteHttpHandler : public IHttpHandler {
  public:
    DeleteHttpHandler(const ServerConfig* serverConfig = 0);
    virtual HttpResponse handle(const HttpRequest& request) const;

  private:
    const ServerConfig* _serverConfig;

    static std::string _resolvePath(const HttpRequest& request,
                                    const ServerConfig* serverConfig,
                                    const LocationConfig* location);
};
