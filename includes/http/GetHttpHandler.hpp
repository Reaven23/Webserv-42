#pragma once

#include "./IHttpHandler.hpp"

class GetHttpHandler : public IHttpHandler {
   public:
    virtual HttpResponse handle(const HttpRequest& request) const;
};
