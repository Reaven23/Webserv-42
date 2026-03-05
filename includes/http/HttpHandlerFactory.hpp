#pragma once

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class HttpHandlerFactory {
 public:
  static HttpResponse handleRequest(const HttpRequest& request);
};
