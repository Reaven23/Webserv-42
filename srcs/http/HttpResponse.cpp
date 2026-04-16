#include "../../includes/http/HttpResponse.hpp"

#include <sstream>

#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/http/DeleteHttpHandler.hpp"
#include "../../includes/http/GetHttpHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/PostHttpHandler.hpp"

using namespace std;

// Constructors
HttpResponse::HttpResponse()
    : version("HTTP/1.1"), statusCode(200), reasonPhrase("OK") {}

HttpResponse::HttpResponse(int statusCode, const string& reasonPhrase)
    : version("HTTP/1.1"), statusCode(statusCode), reasonPhrase(reasonPhrase) {}

// Private methods
static HttpResponse _methodNotAllowedResponse() {
    HttpResponse response(405, "Method Not Allowed");
    response.body = "Method not allowed";
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Allow", "GET, POST, DELETE");

    ostringstream contentLength;
    contentLength << response.body.size();
    response.setHeader("Content-Length", contentLength.str());
    return response;
}

// Setters
HttpResponse& HttpResponse::setHeader(const string& name, const string& value) {
    headers[name] = value;

    return (*this);
}

HttpResponse& HttpResponse::setStatus(int code, const string& reason) {
    statusCode = code;
    reasonPhrase = reason;

    return (*this);
}

HttpResponse& HttpResponse::setBody(string const& bodyContent) {
    body = bodyContent;

    return (*this);
}

// Public methods
HttpResponse HttpResponse::handleRequest(const HttpRequest&  request,
                                         const ServerConfig* serverConfig) {
    switch (request.method) {
        case GET:
            return GetHttpHandler(serverConfig).handle(request);
        case POST:
            return PostHttpHandler(serverConfig).handle(request);
        case DELETE:
            return DeleteHttpHandler(serverConfig).handle(request);
        default:
            return _methodNotAllowedResponse();
    }
}

string HttpResponse::toString() const {
    ostringstream oss;
    oss << version << " " << statusCode << " " << reasonPhrase << "\r\n";

    map<string, string>::const_iterator it = headers.begin();
    for (; it != headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "\r\n";
    oss << body;
    return oss.str();
}
