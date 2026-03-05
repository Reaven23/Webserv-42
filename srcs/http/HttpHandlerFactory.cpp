#include "../../includes/http/HttpHandlerFactory.hpp"

#include "../../includes/http/DeleteHttpHandler.hpp"
#include "../../includes/http/GetHttpHandler.hpp"
#include "../../includes/http/PostHttpHandler.hpp"

#include <sstream>

namespace {
HttpResponse methodNotAllowedResponse() {
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
}  

HttpResponse HttpHandlerFactory::handleRequest(const HttpRequest& request) {
  GetHttpHandler getHandler;
  PostHttpHandler postHandler;
  DeleteHttpHandler deleteHandler;

  switch (request.method) {
    case METHOD_GET:
      return getHandler.handle(request);
    case METHOD_POST:
      return postHandler.handle(request);
    case METHOD_DELETE:
      return deleteHandler.handle(request);
    default:
      return methodNotAllowedResponse();
  }
}
