#include <iostream>
#include <map>
#include <string>

#include "../../includes/http/HttpRequest.hpp"

static void printResult(const ParsedRequest& r) {
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
    std::cout << "query: " << r.request.queryString << std::endl;
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
  ParsedRequest r1 = HttpRequest::parse(raw1);
  printResult(r1);

  // 2) POST avec body complet
  std::string raw2 = "POST /upload?x=1 HTTP/1.1\r\nHost: localhost\r\n"
                     "Content-Length: 11\r\n\r\nhello world";
  ParsedRequest r2 = HttpRequest::parse(raw2);
  printResult(r2);

  // 3) POST incomplet (body tronque)
  std::string raw3 =
      "POST /x HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nabc";
  ParsedRequest r3 = HttpRequest::parse(raw3);
  printResult(r3);

  // 4) Deux requetes collees (pipelining) pour visualiser consumed
  std::string raw4 = "GET /a HTTP/1.1\r\nHost: localhost\r\n\r\n"
                     "GET /b HTTP/1.1\r\nHost: localhost\r\n\r\n";
  ParsedRequest r4a = HttpRequest::parse(raw4);
  printResult(r4a);
  if (r4a.status == PARSE_OK) {
    raw4.erase(0, r4a.consumed);
    ParsedRequest r4b = HttpRequest::parse(raw4);
    printResult(r4b);
  }

  // 5) Chunked valide
  std::string raw5 = "POST /chunk HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Transfer-Encoding: chunked\r\n"
                     "\r\n"
                     "4\r\nWiki\r\n"
                     "5\r\npedia\r\n"
                     "0\r\n"
                     "\r\n";
  ParsedRequest r5 = HttpRequest::parse(raw5);
  printResult(r5);

  // 6) Chunked incomplet
  std::string raw6 = "POST /chunk HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Transfer-Encoding: chunked\r\n"
                     "\r\n"
                     "4\r\nWiki\r\n"
                     "5\r\npedi";
  ParsedRequest r6 = HttpRequest::parse(raw6);
  printResult(r6);

  // 7) Chunked invalide (taille non hex)
  std::string raw7 = "POST /chunk HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Transfer-Encoding: chunked\r\n"
                     "\r\n"
                     "Z\r\nabc\r\n"
                     "0\r\n"
                     "\r\n";
  ParsedRequest r7 = HttpRequest::parse(raw7);
  printResult(r7);

  return 0;
}
