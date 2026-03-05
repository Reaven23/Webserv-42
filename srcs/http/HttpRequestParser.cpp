#include "../includes/http/HttpRequestParser.hpp"
#include <sstream>

namespace {

// transforme la string en enum, genre "GET" -> METHOD_GET
HttpMethod parseMethod(const std::string& s) {
  if (s == "GET") return METHOD_GET;
  if (s == "POST") return METHOD_POST;
  if (s == "DELETE") return METHOD_DELETE;
  return METHOD_UNKNOWN;
}

void splitUriAndQuery(const std::string& full, std::string& uri,
                      std::string& query_string) {
  size_t pos = full.find('?');
  if (pos == std::string::npos) {
    uri = full;
    query_string = "";
  } else {
    uri = full.substr(0, pos);
    query_string = full.substr(pos + 1);
  }
}

// parse les headers (entre la première ligne et \r\n\r\n) et les met dans req
void parseHeaders(const std::string& buffer, size_t headers_start,
                  size_t headers_end, HttpRequest& req) {
  std::string headers_section =
      buffer.substr(headers_start, headers_end - headers_start);

  size_t pos = 0;
  while (pos < headers_section.length()) {
    // on cherche la fin de la ligne courante (\r\n)
    size_t line_end = headers_section.find("\r\n", pos);
    if (line_end == std::string::npos) line_end = headers_section.length();

    // une ligne = un header, genre "Host: localhost"
    std::string line = headers_section.substr(pos, line_end - pos);
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      // avant le : = nom du header, après = valeur
      std::string name = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      // on vire l'espace après le : (format "Name: value")
      if (!value.empty() && value[0] == ' ') value = value.substr(1);
      req.headers[name] = value;
    }

    // on passe à la ligne suivante (+2 pour sauter \r\n)
    if (line_end >= headers_section.length()) break;
    pos = line_end + 2;
  }
}

}  // namespace

// on reçoit le buffer brut du socket et on tente d'en sortir une requête complète.
// pas encore \r\n\r\n ? on attend plus de données. sinon on découpe la première
// ligne (méthode uri version), on parse, et on renvoie la requête + combien
// d'octets on a mangé pour que l'appelant puisse les virer du buffer.

// ex. requête raw (GET) : "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
// ex. requête raw (POST avec body) : "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\nhello world"
ParseResult HttpRequestParser::parse(const std::string& buffer) {
  ParseResult result;
  result.status = PARSE_INCOMPLETE;
  result.consumed = 0;

  // pas de \r\n\r\n = on a pas fini de recevoir, on attend la suite
  size_t headers_end = buffer.find("\r\n\r\n");
  if (headers_end == std::string::npos) return result;

  // première ligne = tout jusqu'au premier \r\n
  size_t line_end = buffer.find("\r\n");
  if (line_end == std::string::npos) return result;

  std::string request_line = buffer.substr(0, line_end);
  // ça renvoit un truc genre : "GET /index.html HTTP/1.1" pour un get

  // on split par espaces : "GET /index.html HTTP/1.1" -> method, uri, version
  std::istringstream iss(request_line);
  std::string method_str, uri_full, version;
  iss >> method_str >> uri_full >> version;

  // une des trois parties vide = requête nulle, on renvoie erreur
  if (method_str.empty() || uri_full.empty() || version.empty()) {
    result.status = PARSE_ERROR;
    return result;
  }

  // version qui ressemble pas à HTTP/x.y (genre HTTPPP ou nimp) -> erreur
  // pas sur que ce soit très utile si on a que des vrais client ou des curls
  // mais bon on sait jamais.
  if (version.length() < 8 || version.substr(0, 5) != "HTTP/") {
    result.status = PARSE_ERROR;
    return result;
  }

  result.request.method = parseMethod(method_str);
  splitUriAndQuery(uri_full, result.request.uri, result.request.query_string);
  result.request.version = version;

  // méthode qu'on gère pas (PUT, PATCH, etc.) -> erreur
  if (result.request.method == METHOD_UNKNOWN) {
    result.status = PARSE_ERROR;
    return result;
  }

  // parse les headers (entre la fin de la 1ère ligne et \r\n\r\n)
  parseHeaders(buffer, line_end + 2, headers_end, result.request);

  // body : le body commence juste après \r\n\r\n
  size_t body_start = headers_end + 4;
  size_t content_length = 0;
  // si y'a un Content-Length, on sait combien d'octets attendre
  if (result.request.headers.find("Content-Length") != result.request.headers.end()) {
    std::istringstream cl_iss(result.request.headers["Content-Length"]);
    cl_iss >> content_length;
  }

  // pas assez de données dans le buffer ? on attend la suite
  if (buffer.length() < body_start + content_length) {
    result.status = PARSE_INCOMPLETE;
    return result;
  }

  // on extrait le body et on dit combien on a bouffé
  result.request.body = buffer.substr(body_start, content_length);
  result.consumed = body_start + content_length;
  result.status = PARSE_OK;

  return result;
}
