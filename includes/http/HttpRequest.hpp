#pragma once

#include <cstddef>
#include <map>
#include <string>

enum HttpMethod { GET, POST, DELETE, UNKNOWN };

enum ParseStatus { OK, INCOMPLETE, ERROR };

struct ParsedHttpRequest;

class HttpRequest {
   public:
    HttpMethod                         method;
    std::string                        uri;
    std::string                        queryString;
    std::string                        version;
    std::map<std::string, std::string> headers;
    std::string                        body;

    HttpRequest() : method(UNKNOWN) {}

    static ParsedHttpRequest parse(const std::string& buffer);

   private:
    static HttpMethod _parseMethod(const std::string& s);
    static void _splitUriAndQuery(const std::string& full, std::string& uri,
                                  std::string& queryString);
    static bool _parseHeaders(const std::string& buffer, size_t headers_start,
                              size_t headers_end, HttpRequest& req);
    static std::string _trim(const std::string& s);
    static std::string _toLower(const std::string& s);
    static bool _parseContentLength(const std::string& value, size_t& out);
    static bool _hasToken(const std::string& csvValue,
                          const std::string& token);
    static ParseStatus _parseChunkedBody(const std::string& buffer,
                                         size_t             body_start,
                                         std::string&       outBody,
                                         size_t&            outConsumed);
};

struct ParsedHttpRequest {
    ParseStatus status;
    HttpRequest request;
    size_t      consumed;
};
