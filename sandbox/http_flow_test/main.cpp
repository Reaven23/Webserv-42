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

    ParsedHttpRequest parsed = HttpRequest::parse(raw);
    std::cout << "[PARSE STATUS] ";
    if (parsed.status == OK)
        std::cout << "OK";
    else if (parsed.status == INCOMPLETE)
        std::cout << "INCOMPLETE";
    else
        std::cout << "ERROR";
    std::cout << "\n";
    std::cout << "[CONSUMED] " << parsed.consumed << "\n";

    if (parsed.status != OK) return;

    HttpResponse response = HttpResponse::handleRequest(parsed.request);
    std::string  wire = response.toString();

    std::cout << "[RESPONSE OBJECT] status=" << response.statusCode << " "
              << response.reasonPhrase << "\n";
    std::cout << "[RAW RESPONSE]\n" << wire << "\n";
}

static void runPipeline() {
    printSection("PIPELINE TWO REQUESTS");
    std::string buffer =
        "GET /one HTTP/1.1\r\nHost: localhost\r\n\r\n"
        "DELETE /two HTTP/1.1\r\nHost: localhost\r\n\r\n";

    ParsedHttpRequest first = HttpRequest::parse(buffer);
    std::cout << "[FIRST] status="
              << (first.status == OK
                      ? "OK"
                      : (first.status == INCOMPLETE ? "INCOMPLETE" : "ERROR"))
              << " consumed=" << first.consumed << "\n";
    if (first.status == OK) {
        HttpResponse r1 = HttpResponse::handleRequest(first.request);
        std::cout << "[FIRST RESPONSE]\n" << r1.toString() << "\n";
        buffer.erase(0, first.consumed);
    }

    ParsedHttpRequest second = HttpRequest::parse(buffer);
    std::cout << "[SECOND] status="
              << (second.status == OK
                      ? "OK"
                      : (second.status == INCOMPLETE ? "INCOMPLETE" : "ERROR"))
              << " consumed=" << second.consumed << "\n";
    if (second.status == OK) {
        HttpResponse r2 = HttpResponse::handleRequest(second.request);
        std::cout << "[SECOND RESPONSE]\n" << r2.toString() << "\n";
    }
}

int main() {
    runOne("GET BASIC", "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
    runOne("GET HOME FALLBACK",
           "GET /home HTTP/1.1\r\nHost: localhost\r\n\r\n");

    runOne("POST BASIC",
           "POST /upload?x=42 HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
           "11\r\n\r\nhello world");

    runOne("DELETE BASIC",
           "DELETE /old/file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n");

    runOne("METHOD NOT ALLOWED", "PUT /x HTTP/1.1\r\nHost: localhost\r\n\r\n");

    runOne("INCOMPLETE BODY",
           "POST /x HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
           "10\r\n\r\nabc");

    runPipeline();
    return 0;
}
