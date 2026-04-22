#include "../../includes//http/SessionHandler.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "../../includes/http/CookieUtils.hpp"
#include "../../includes/http/SessionManager.hpp"

using namespace std;

static const string PAGES_DIR = "www/html/session/";

static string readFile(const string& path) {
    ifstream file(path.c_str(), ios::binary);
    if (!file.is_open()) return ("");

    ostringstream oss;
    oss << file.rdbuf();

    return (oss.str());
}

static string replaceAll(string const& src, const string& placeholder,
                         string const& value) {
    string result = src;
    size_t pos = 0;

    while ((pos = result.find(placeholder, pos)) != string::npos) {
        result.replace(pos, placeholder.size(), value);
        pos += value.size();
    }

    return result;
}

static string extractUsername(string const& body) {
    size_t pos = body.find("username=");

    if (pos == string::npos) return ("");

    size_t start = pos + 9;
    size_t end = body.find('&', start);

    if (end == string::npos) end = body.size();

    string val = body.substr(start, end - start);
    for (size_t i = 0; i < val.size(); i++) {
        if (val[i] == '+') val[i] = ' ';
    }
    return (val);
}

static HttpResponse htmlResponse(int code, string const& reason,
                                 string const& body) {
    HttpResponse response(code, reason);

    ostringstream os;
    os << body.size();
    response.setBody(body)
        .setHeader("Content-Type", "text/html")
        .setHeader("Content-Length", os.str());

    return (response);
}

bool SessionHandler::isSessionRoute(string const& uri) {
    return (uri == "/session" || uri == "/session/logout");
}

HttpResponse SessionHandler::handle(HttpRequest const& request) {
    string                              sessionId;
    map<string, string>::const_iterator it = request.headers.find("cookie");

    if (it != request.headers.end())
        sessionId = CookieUtils::getValue(it->second, "session_id");

    if (request.uri == "/session/logout") return (_handleLogout(sessionId));

    if (request.method == POST) return (_handleLogin(request));

    Session* session = SessionManager::get(sessionId);
    if (session) return (_dashboard(sessionId));

    return (_loginPage());
}

HttpResponse SessionHandler::_loginPage() {
    string body = readFile(PAGES_DIR + "login.html");

    if (body.empty())
        return (
            htmlResponse(500, "Internal Server Error", "Could not load page"));
    return (htmlResponse(200, "OK", body));
};

HttpResponse SessionHandler::_handleLogin(HttpRequest const& request) {
    string username = extractUsername(request.body);

    if (username.empty()) return (_loginPage());

    string sessionId = SessionManager::create();
    SessionManager::set(sessionId, "username", username);

    HttpResponse response(303, "See Other");

    response.setHeader("Location", "/session")
        .setHeader("Content-Length", "0")
        .setCookie("session_id=" + sessionId + "; Path=/; HttpOnly");

    return response;
}

HttpResponse SessionHandler::_dashboard(string const& sessionId) {
    Session* session = SessionManager::get(sessionId);

    if (!session) return (_loginPage());

    string body = readFile(PAGES_DIR + "dashboard.html");
    if (body.empty())
        return (
            htmlResponse(500, "Internal Server Error", "Could not load page"));

    body = replaceAll(body, "{{username}}", session->data["username"]);
    body = replaceAll(body, "{{session_id}}", sessionId);

    return (htmlResponse(200, "OK", body));
}

HttpResponse SessionHandler::_handleLogout(string const& sessionId) {
    if (!sessionId.empty()) SessionManager::destroy(sessionId);

    HttpResponse response(303, "See Other");

    response.setHeader("Location", "/session")
        .setHeader("Content-Length", "0")
        .setCookie("session_id=; Path=/; HttpOnly; Max-Age=0");

    return (response);
}
