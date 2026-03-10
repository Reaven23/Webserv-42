#include "../../includes/http/HttpResponse.hpp"

#include <sstream>

#include "../../includes/http/DeleteHttpHandler.hpp"
#include "../../includes/http/GetHttpHandler.hpp"
#include "../../includes/http/PostHttpHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"

HttpResponse::HttpResponse() : statusCode(200), reasonPhrase("OK") {}

HttpResponse::HttpResponse(int statusCode, const std::string& reasonPhrase)
    : statusCode(statusCode), reasonPhrase(reasonPhrase) {}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
  headers[name] = value;
}

std::string HttpResponse::toString() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << statusCode << " " << reasonPhrase << "\r\n";

  std::map<std::string, std::string>::const_iterator it = headers.begin();
  for (; it != headers.end(); ++it) {
    oss << it->first << ": " << it->second << "\r\n";
  }

  oss << "\r\n";
  oss << body;
  return oss.str();
}

void HttpResponse::setStatus(int code, const std::string& reason) {
  statusCode = code;
  reasonPhrase = reason;
}

static HttpResponse _methodNotAllowedResponse() {
  HttpResponse response(405, "Method Not Allowed");
  response.body = "Method not allowed";
  response.setHeader("Content-Type", "text/plain");
  response.setHeader("Allow", "GET, POST, DELETE");

  std::ostringstream contentLength;
  contentLength << response.body.size();
  response.setHeader("Content-Length", contentLength.str());
  response.setHeader("Connection", "close");
  return response;
}

HttpResponse HttpResponse::handleRequest(const HttpRequest& request) {
  switch (request.method) {
    case GET:
      return GetHttpHandler().handle(request);
    case POST:
      return PostHttpHandler().handle(request);
    case DELETE:
      return DeleteHttpHandler().handle(request);
    default:
      return _methodNotAllowedResponse();
  }
}
