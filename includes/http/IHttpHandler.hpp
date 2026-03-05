#pragma once

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class IHttpHandler {
 public:
  virtual ~IHttpHandler() {}
  virtual HttpResponse handle(const HttpRequest& request) const = 0;
};
