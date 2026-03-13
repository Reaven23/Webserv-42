#include <iostream>
#include <string>

#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/HttpResponse.hpp"

static void printSection(const std::string& title) {
  std::cout << "\n========== " << title << " ==========\n";
}

static void runOne(const std::string& name, const std::string& raw) {
  printSection(name);
  std::cout << "[RAW REQUEST]\n" << raw << "\n";

  ParsedRequest parsed = HttpRequest::parse(raw);
  std::cout << "[PARSE STATUS] ";
  if (parsed.status == PARSE_OK)
    std::cout << "PARSE_OK";
  else if (parsed.status == PARSE_INCOMPLETE)
    std::cout << "PARSE_INCOMPLETE";
  else
    std::cout << "PARSE_ERROR";
  std::cout << "\n";
  std::cout << "[CONSUMED] " << parsed.consumed << "\n";

  if (parsed.status != PARSE_OK) return;

  HttpResponse response = HttpResponse::handleRequest(parsed.request);
  std::string wire = response.toString();

  std::cout << "[RESPONSE OBJECT] status=" << response.statusCode << " "
            << response.reasonPhrase << "\n";
  std::cout << "[RAW RESPONSE]\n" << wire << "\n";
}

static void runPipeline() {
  printSection("PIPELINE TWO REQUESTS");
  std::string buffer = "GET /one HTTP/1.1\r\nHost: localhost\r\n\r\n"
                       "DELETE /two HTTP/1.1\r\nHost: localhost\r\n\r\n";

  ParsedRequest first = HttpRequest::parse(buffer);
  std::cout << "[FIRST] status="
            << (first.status == PARSE_OK ? "OK"
                                         : (first.status == PARSE_INCOMPLETE
                                                ? "INCOMPLETE"
                                                : "ERROR"))
            << " consumed=" << first.consumed << "\n";
  if (first.status == PARSE_OK) {
    HttpResponse r1 = HttpResponse::handleRequest(first.request);
    std::cout << "[FIRST RESPONSE]\n" << r1.toString() << "\n";
    buffer.erase(0, first.consumed);
  }

  ParsedRequest second = HttpRequest::parse(buffer);
  std::cout << "[SECOND] status="
            << (second.status == PARSE_OK ? "OK"
                                          : (second.status == PARSE_INCOMPLETE
                                                 ? "INCOMPLETE"
                                                 : "ERROR"))
            << " consumed=" << second.consumed << "\n";
  if (second.status == PARSE_OK) {
    HttpResponse r2 = HttpResponse::handleRequest(second.request);
    std::cout << "[SECOND RESPONSE]\n" << r2.toString() << "\n";
  }
}

int main() {
  runOne("GET BASIC",
         "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");

  runOne("POST BASIC",
         "POST /upload?x=42 HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
         "11\r\n\r\nhello world");

  runOne("DELETE BASIC",
         "DELETE /old/file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n");

  runOne("METHOD NOT ALLOWED",
         "PUT /x HTTP/1.1\r\nHost: localhost\r\n\r\n");

  runOne("INCOMPLETE BODY",
         "POST /x HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
         "10\r\n\r\nabc");

  runPipeline();
  return 0;
}
