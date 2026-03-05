#include "../../includes/http/HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse() : status_code(200), reason_phrase("OK") {}

HttpResponse::HttpResponse(int statusCode, const std::string& reasonPhrase)
    : status_code(statusCode), reason_phrase(reasonPhrase) {}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
  headers[name] = value;
}

std::string HttpResponse::toString() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << " " << reason_phrase << "\r\n";

  std::map<std::string, std::string>::const_iterator it = headers.begin();
  for (; it != headers.end(); ++it) {
    oss << it->first << ": " << it->second << "\r\n";
  }

  oss << "\r\n";
  oss << body;
  return oss.str();
}
