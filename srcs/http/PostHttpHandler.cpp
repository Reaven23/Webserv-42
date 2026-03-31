#include "../../includes/http/PostHttpHandler.hpp"
#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"

#include <sys/stat.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

PostHttpHandler::PostHttpHandler(const ServerConfig* serverConfig)
    : _serverConfig(serverConfig) {}

bool PostHttpHandler::_isMultipart(const HttpRequest& request,
                                    std::string& boundary) {
    std::map<std::string, std::string>::const_iterator it =
        request.headers.find("content-type");
    if (it == request.headers.end())
        return false;

    const std::string& ct = it->second;
    if (ct.compare(0, 19, "multipart/form-data") != 0)
        return false;

    boundary = _extractBoundary(ct);
    return !boundary.empty();
}

std::string PostHttpHandler::_extractBoundary(const std::string& contentType) {
    std::string::size_type pos = contentType.find("boundary=");
    if (pos == std::string::npos)
        return "";

    std::string boundary = contentType.substr(pos + 9);

    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1);
        std::string::size_type end = boundary.find('"');
        if (end != std::string::npos)
            boundary = boundary.substr(0, end);
    }

    std::string::size_type semi = boundary.find(';');
    if (semi != std::string::npos)
        boundary = boundary.substr(0, semi);

    while (!boundary.empty() && boundary[boundary.size() - 1] == ' ')
        boundary.erase(boundary.size() - 1);

    return boundary;
}

std::vector<PostHttpHandler::UploadedFile>
PostHttpHandler::_parseMultipart(const std::string& body,
                                  const std::string& boundary) {
    std::vector<UploadedFile> files;
    std::string delim = "--" + boundary;
    std::string endDelim = delim + "--";

    std::string::size_type pos = body.find(delim);
    if (pos == std::string::npos)
        return files;
    pos += delim.size();

    while (true) {
        if (pos < body.size() && body[pos] == '\r') ++pos;
        if (pos < body.size() && body[pos] == '\n') ++pos;

        std::string::size_type nextDelim = body.find(delim, pos);
        if (nextDelim == std::string::npos)
            break;

        std::string part = body.substr(pos, nextDelim - pos);

        std::string::size_type headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            pos = nextDelim + delim.size();
            if (body.compare(nextDelim, endDelim.size(), endDelim) == 0)
                break;
            continue;
        }

        std::string headers = part.substr(0, headerEnd);
        std::string data = part.substr(headerEnd + 4);

        if (data.size() >= 2 &&
            data[data.size() - 2] == '\r' && data[data.size() - 1] == '\n')
            data.erase(data.size() - 2);

        std::string filename;
        std::string::size_type fnPos = headers.find("filename=\"");
        if (fnPos != std::string::npos) {
            fnPos += 10;
            std::string::size_type fnEnd = headers.find('"', fnPos);
            if (fnEnd != std::string::npos)
                filename = headers.substr(fnPos, fnEnd - fnPos);
        }

        std::string ct;
        std::string::size_type ctPos = headers.find("Content-Type: ");
        if (ctPos != std::string::npos) {
            ctPos += 14;
            std::string::size_type ctEnd = headers.find("\r\n", ctPos);
            ct = (ctEnd != std::string::npos)
                     ? headers.substr(ctPos, ctEnd - ctPos)
                     : headers.substr(ctPos);
        }

        if (!filename.empty()) {
            UploadedFile file;
            file.filename = _sanitizeFilename(filename);
            file.contentType = ct;
            file.data = data;
            if (!file.filename.empty())
                files.push_back(file);
        }

        pos = nextDelim + delim.size();
        if (body.compare(nextDelim, endDelim.size(), endDelim) == 0)
            break;
    }

    return files;
}

std::string PostHttpHandler::_sanitizeFilename(const std::string& raw) {
    std::string name = raw;

    std::string::size_type slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);

    std::string safe;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (c == '\0' || c == '/' || c == '\\')
            continue;
        if (c == '.' && safe == ".")
            continue;
        safe += c;
    }

    if (safe.empty() || safe == "." || safe == "..") {
        std::ostringstream fallback;
        fallback << "upload_" << static_cast<unsigned long>(time(NULL));
        return fallback.str();
    }
    return safe;
}

HttpResponse PostHttpHandler::handle(const HttpRequest& request) const {
    if (request.uri.empty() || request.uri[0] != '/')
        return _errorResponse(400, "Bad Request", _serverConfig);

    std::string decoded = _percentDecode(request.uri);
    if (decoded.empty())
        return _errorResponse(400, "Bad Request", _serverConfig);

    std::string cleanUri = _normalizePath(decoded);
    if (cleanUri.empty())
        return _errorResponse(403, "Forbidden", _serverConfig);

    HttpRequest sanitized = request;
    sanitized.uri = cleanUri;

    const LocationConfig* location = _findLocation(sanitized.uri, _serverConfig);

    if (location != NULL) {
        const std::vector<std::string>& methods = location->getMethods();
        if (!methods.empty()) {
            bool allowed = false;
            for (size_t i = 0; i < methods.size(); ++i) {
                if (methods[i] == "POST") {
                    allowed = true;
                    break;
                }
            }
            if (!allowed)
                return _methodNotAllowedResponse(methods);
        }
    }

    if (_serverConfig != NULL) {
        size_t maxBody = _serverConfig->getClientMaxBodySize();
        if (maxBody > 0 && request.body.size() > maxBody)
            return _errorResponse(413, "Payload Too Large", _serverConfig);
    }

    if (location == NULL || !location->hasUploadDirective() ||
        !location->getUploadEnabled())
        return _errorResponse(403, "Forbidden", _serverConfig);

    std::string uploadDir = location->getUploadPath();
    if (uploadDir.empty())
        return _errorResponse(500, "Internal Server Error", _serverConfig);

    struct stat dirStat = {};
    if (stat(uploadDir.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode))
        return _errorResponse(500, "Internal Server Error", _serverConfig);

    std::string boundary;
    if (_isMultipart(request, boundary)) {
        std::vector<UploadedFile> files = _parseMultipart(request.body, boundary);
        if (files.empty())
            return _errorResponse(400, "Bad Request", _serverConfig);

        std::ostringstream resultBody;
        for (size_t i = 0; i < files.size(); ++i) {
            std::string filePath = uploadDir + "/" + files[i].filename;

            std::ofstream out(filePath.c_str(),
                              std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out.is_open())
                return _errorResponse(500, "Internal Server Error", _serverConfig);

            out.write(files[i].data.c_str(),
                      static_cast<std::streamsize>(files[i].data.size()));
            if (!out.good()) {
                out.close();
                std::remove(filePath.c_str());
                return _errorResponse(500, "Internal Server Error", _serverConfig);
            }
            out.close();

            if (i > 0) resultBody << "\n";
            resultBody << files[i].filename
                       << " (" << files[i].data.size() << " bytes)";
        }

        HttpResponse response(201, "Created");
        response.setBody(resultBody.str());
        response.setHeader("Content-Type", "text/plain");

        std::ostringstream cl;
        cl << response.body.size();
        response.setHeader("Content-Length", cl.str());
        response.setHeader("Connection", "close");
        return response;
    }

    std::string relPath = sanitized.uri.substr(location->getPath().size());
    if (!relPath.empty() && relPath[0] == '/')
        relPath = relPath.substr(1);

    if (relPath.empty()) {
        std::ostringstream name;
        name << "upload_" << static_cast<unsigned long>(time(NULL));
        relPath = name.str();
    }

    std::string filePath = uploadDir + "/" + relPath;

    std::ofstream outFile(filePath.c_str(),
                          std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
        return _errorResponse(500, "Internal Server Error", _serverConfig);

    outFile.write(request.body.c_str(),
                  static_cast<std::streamsize>(request.body.size()));
    if (!outFile.good()) {
        outFile.close();
        std::remove(filePath.c_str());
        return _errorResponse(500, "Internal Server Error", _serverConfig);
    }
    outFile.close();

    std::ostringstream bodyMsg;
    bodyMsg << "File uploaded: " << relPath
            << " (" << request.body.size() << " bytes)";

    HttpResponse response(201, "Created");
    response.setBody(bodyMsg.str());
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Location", sanitized.uri);

    std::ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}
