#pragma once

#include "./IHttpHandler.hpp"

class DeleteHttpHandler : public IHttpHandler {
   public:
    virtual HttpResponse handle(const HttpRequest& request) const;
};
