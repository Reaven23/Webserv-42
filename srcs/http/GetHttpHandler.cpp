#include "../../includes/http/GetHttpHandler.hpp"

#include <sstream>

HttpResponse GetHttpHandler::handle(const HttpRequest& request) const {
  HttpResponse response(200, "OK");
  std::ostringstream body;
  body << "GET handled for " << request.uri;
  response.body = body.str();
  response.setHeader("Content-Type", "text/plain");

  std::ostringstream contentLength;
  contentLength << response.body.size();
  response.setHeader("Content-Length", contentLength.str());
  response.setHeader("Connection", "close");
  return response;
}
