#pragma once

#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"

class IHttpHandler {
   public:
    virtual ~IHttpHandler() {}
    virtual HttpResponse handle(const HttpRequest& request) const = 0;
};
