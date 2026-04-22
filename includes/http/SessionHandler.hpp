#pragma once

#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"

class SessionHandler {
   public:
    static bool         isSessionRoute(std::string const& uri);
    static HttpResponse handle(HttpRequest const& request);

   private:
    static HttpResponse _loginPage();
    static HttpResponse _handleLogin(HttpRequest const& request);
    static HttpResponse _dashboard(std::string const& sessionId);
    static HttpResponse _handleLogout(std::string const& sessionId);
};
