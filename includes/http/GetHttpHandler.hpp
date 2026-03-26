#pragma once

#include <sys/stat.h>

#include "./IHttpHandler.hpp"

class GetHttpHandler : public IHttpHandler {
   public:
    virtual HttpResponse handle(const HttpRequest& request) const;

   private:
    static HttpResponse textResponse(int code, const std::string& reason,
                                     const std::string& body);
    static std::string  contentTypeFromPath(const std::string& path);
    static std::string  resolvePath(const HttpRequest& request);
    static bool         fileExists(const std::string& path, struct stat* st);
    static bool         isReadableFile(const std::string& path);
};
