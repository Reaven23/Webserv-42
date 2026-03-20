#pragma once

#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"

class HttpHandlerFactory {
   public:
    static HttpResponse handleRequest(const HttpRequest& request);
};
