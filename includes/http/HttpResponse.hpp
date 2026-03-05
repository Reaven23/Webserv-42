#pragma once

#include <map>
#include <string>

class HttpResponse {
 public:
  int status_code;
  std::string reason_phrase;
  std::map<std::string, std::string> headers;
  std::string body;

  HttpResponse();
  HttpResponse(int statusCode, const std::string& reasonPhrase);

  void setHeader(const std::string& name, const std::string& value);
  std::string toString() const;
};
