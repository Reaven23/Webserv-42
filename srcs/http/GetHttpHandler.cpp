#include "../../includes/http/GetHttpHandler.hpp"

#include <unistd.h>
#include <fstream>
#include <sstream>

HttpResponse GetHttpHandler::textResponse(int code, const std::string& reason,
                                          const std::string& body) {
  HttpResponse response(code, reason);
  response.body = body;
  response.setHeader("Content-Type", "text/plain");
  std::ostringstream contentLength;
  contentLength << response.body.size();
  response.setHeader("Content-Length", contentLength.str());
  response.setHeader("Connection", "close");
  return response;
}

std::string GetHttpHandler::contentTypeFromPath(const std::string& path) {
  if (path.size() >= 5 && path.substr(path.size() - 5) == ".html")
    return "text/html";
  if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")
    return "text/css";
  if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
    return "application/javascript";
  if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")
    return "image/png";
  if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")
    return "image/jpeg";
  if (path.size() >= 5 && path.substr(path.size() - 5) == ".jpeg")
    return "image/jpeg";
  return "application/octet-stream";
}

bool GetHttpHandler::fileExists(const std::string& path, struct stat* st) {
  return stat(path.c_str(), st) == 0;
}

bool GetHttpHandler::isReadableFile(const std::string& path) {
  return access(path.c_str(), R_OK) == 0;
}

std::string GetHttpHandler::resolvePath(const HttpRequest& request) {
  std::string uriPath = request.uri;
  if (uriPath == "/") return "./index.html";

  std::string direct = "." + uriPath;
  struct stat st = {};
  if (fileExists(direct, &st) && S_ISREG(st.st_mode)) return direct;

  std::string indexCandidate = direct;
  if (!indexCandidate.empty() && indexCandidate[indexCandidate.size() - 1] != '/')
    indexCandidate += "/";
  indexCandidate += "index.html";
  if (fileExists(indexCandidate, &st) && S_ISREG(st.st_mode))
    return indexCandidate;

  std::string htmlCandidate = direct + ".html";
  if (fileExists(htmlCandidate, &st) && S_ISREG(st.st_mode))
    return htmlCandidate;

  return direct;
}

HttpResponse GetHttpHandler::handle(const HttpRequest& request) const {
  if (request.uri.empty() || request.uri[0] != '/')
    return GetHttpHandler::textResponse(400, "Bad Request", "Invalid URI");
  if (request.uri.find("..") != std::string::npos)
    return GetHttpHandler::textResponse(403, "Forbidden", "Forbidden path");

  std::string path = GetHttpHandler::resolvePath(request);

  struct stat st = {};
  if (!GetHttpHandler::fileExists(path, &st))
    return GetHttpHandler::textResponse(404, "Not Found", "Not Found");
  if (!S_ISREG(st.st_mode))
    return GetHttpHandler::textResponse(403, "Forbidden",
                                        "Target is not a regular file");
  if (!GetHttpHandler::isReadableFile(path))
    return GetHttpHandler::textResponse(403, "Forbidden", "Read forbidden");

  std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open())
    return GetHttpHandler::textResponse(500, "Internal Server Error",
                                        "Open error");

  std::ostringstream body;
  body << file.rdbuf();
  if (!file.good() && !file.eof())
    return GetHttpHandler::textResponse(500, "Internal Server Error",
                                        "Read error");

  HttpResponse response(200, "OK");
  response.body = body.str();
  response.setHeader("Content-Type", GetHttpHandler::contentTypeFromPath(path));
  std::ostringstream contentLength;
  contentLength << response.body.size();
  response.setHeader("Content-Length", contentLength.str());
  response.setHeader("Connection", "close");
  return response;
}
