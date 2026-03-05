#pragma once

#include "http/HttpRequest.hpp"
#include <cstddef>
#include <string>

enum ParseStatus {
  PARSE_OK,
  PARSE_INCOMPLETE,
  PARSE_ERROR
};

struct ParseResult {
  ParseStatus status;
  HttpRequest request;
  size_t consumed;
};

class HttpRequestParser {
 public:
  static ParseResult parse(const std::string& buffer);
};
