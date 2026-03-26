#include "../../includes/http/PostHttpHandler.hpp"

#include <sstream>

HttpResponse PostHttpHandler::handle(const HttpRequest& request) const {
  HttpResponse response(200, "OK");
  std::ostringstream body;
  body << "POST handled for " << request.uri << " (body_size="
       << request.body.size() << ")";
  response.body = body.str();
  response.setHeader("Content-Type", "text/plain");

  std::ostringstream contentLength;
  contentLength << response.body.size();
  response.setHeader("Content-Length", contentLength.str());
  response.setHeader("Connection", "close");
  return response;
}
