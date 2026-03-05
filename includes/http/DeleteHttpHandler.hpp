#pragma once

#include "http/IHttpHandler.hpp"

class DeleteHttpHandler : public IHttpHandler {
 public:
  virtual HttpResponse handle(const HttpRequest& request) const;
};
