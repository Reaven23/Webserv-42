#pragma once

#include "http/IHttpHandler.hpp"

class PostHttpHandler : public IHttpHandler {
 public:
  virtual HttpResponse handle(const HttpRequest& request) const;
};
