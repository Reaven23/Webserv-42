#include <iostream>
#include <map>
#include <string>

#include "../../includes/http/HttpRequestParser.hpp"

static void printResult(const ParseResult& r) {
  std::cout << "status: ";
  if (r.status == PARSE_OK)
    std::cout << "PARSE_OK";
  else if (r.status == PARSE_INCOMPLETE)
    std::cout << "PARSE_INCOMPLETE";
  else
    std::cout << "PARSE_ERROR";
  std::cout << std::endl;

  std::cout << "consumed: " << r.consumed << std::endl;

  if (r.status == PARSE_OK) {
    std::cout << "method(enum): " << r.request.method << std::endl;
    std::cout << "uri: " << r.request.uri << std::endl;
    std::cout << "query: " << r.request.query_string << std::endl;
    std::cout << "version: " << r.request.version << std::endl;
    std::cout << "headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it =
             r.request.headers.begin();
         it != r.request.headers.end(); ++it) {
      std::cout << "  " << it->first << " = " << it->second << std::endl;
    }
    std::cout << "body: [" << r.request.body << "]" << std::endl;
  }
  std::cout << "-----------------------------" << std::endl;
}

int main() {
  // 1) GET simple
  std::string raw1 = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
  ParseResult r1 = HttpRequestParser::parse(raw1);
  printResult(r1);

  // 2) POST avec body complet
  std::string raw2 = "POST /upload?x=1 HTTP/1.1\r\nHost: localhost\r\n"
                     "Content-Length: 11\r\n\r\nhello world";
  ParseResult r2 = HttpRequestParser::parse(raw2);
  printResult(r2);

  // 3) POST incomplet (body tronque)
  std::string raw3 =
      "POST /x HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nabc";
  ParseResult r3 = HttpRequestParser::parse(raw3);
  printResult(r3);

  // 4) Deux requetes collees (pipelining) pour visualiser consumed
  std::string raw4 = "GET /a HTTP/1.1\r\nHost: localhost\r\n\r\n"
                     "GET /b HTTP/1.1\r\nHost: localhost\r\n\r\n";
  ParseResult r4a = HttpRequestParser::parse(raw4);
  printResult(r4a);
  if (r4a.status == PARSE_OK) {
    raw4.erase(0, r4a.consumed);
    ParseResult r4b = HttpRequestParser::parse(raw4);
    printResult(r4b);
  }

  return 0;
}
