#pragma once

#include <map>
#include <string>

enum HttpMethod {
  METHOD_GET,
  METHOD_POST,
  METHOD_DELETE,
  METHOD_UNKNOWN
};

class HttpRequest {
 public:
  HttpMethod method;
  std::string uri;            // ex: "/index.html" ou "/search" (sans le ?...)
  std::string query_string;   // ex: "q=hello&page=2" ou vide
  // On sépare la string en deux (uri et query string pour faciliter le
  // passage ensuite au cgi (pas tout capté encore le CGI mais visiblement
  // il a besoin de la query séparé du reste de lurl))
  std::string version;         // ex: "HTTP/1.1"
  std::map<std::string, std::string> headers;
  std::string body;

  HttpRequest() : method(METHOD_UNKNOWN) {}
};
