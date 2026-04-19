#pragma once

#include <map>
#include <string>
#include <vector>

class HttpRequest;
class ServerConfig;

class HttpResponse {
   public:
    std::string                        version;
    int                                statusCode;
    std::string                        reasonPhrase;
    std::map<std::string, std::string> headers;
    std::string                        body;

    HttpResponse();
    HttpResponse(int statusCode, const std::string& reasonPhrase);

    std::string toString() const;

    HttpResponse& setHeader(const std::string& name, const std::string& value);
    HttpResponse& setCookie(std::string const& value);
    HttpResponse& setStatus(int code, const std::string& reason);
    HttpResponse& setBody(std::string const& body);

    static HttpResponse handleRequest(const HttpRequest&  request,
                                      const ServerConfig* serverConfig = 0);

   private:
    std::vector<std::string> _cookies;
};
