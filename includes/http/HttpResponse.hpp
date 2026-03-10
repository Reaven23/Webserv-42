#pragma once

#include <map>
#include <string>

class HttpRequest;

class HttpResponse {
 public:
  int statusCode;
  std::string reasonPhrase;
  std::map<std::string, std::string> headers;
  std::string body;

  HttpResponse();
  HttpResponse(int statusCode, const std::string& reasonPhrase);

  void setHeader(const std::string& name, const std::string& value);
  std::string toString() const;

  void setStatus(int code, const std::string& reason);


  static HttpResponse handleRequest(const HttpRequest& request);
};
