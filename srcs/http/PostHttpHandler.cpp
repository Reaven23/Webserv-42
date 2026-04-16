#include "../../includes/http/PostHttpHandler.hpp"

#include <sys/stat.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"

using namespace std;

PostHttpHandler::PostHttpHandler(const ServerConfig* serverConfig)
    : _serverConfig(serverConfig) {}

bool PostHttpHandler::_extractBoundary(const string& contentType,
                                       string&       boundary) {
    boundary.clear();
    string::size_type pos = contentType.find("boundary=");
    if (pos == string::npos) return false;

    boundary = contentType.substr(pos + 9);

    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1);
        string::size_type end = boundary.find('"');
        if (end == string::npos) return false;
        boundary = boundary.substr(0, end);
    }

    string::size_type semi = boundary.find(';');
    if (semi != string::npos) boundary = boundary.substr(0, semi);

    while (!boundary.empty() && boundary[boundary.size() - 1] == ' ')
        boundary.erase(boundary.size() - 1);

    return !boundary.empty();
}

vector<PostHttpHandler::UploadedFile> PostHttpHandler::_parseMultipart(
    const string& body, const string& boundary, bool& malformed) {
    malformed = false;
    vector<UploadedFile> files;
    string               delim = "--" + boundary;
    string               endDelim = delim + "--";

    string::size_type pos = body.find(delim);
    if (pos == string::npos) return files;
    pos += delim.size();

    while (true) {
        if (pos < body.size() && body[pos] == '\r') ++pos;
        if (pos < body.size() && body[pos] == '\n') ++pos;

        string::size_type nextDelim = body.find(delim, pos);
        if (nextDelim == string::npos) break;

        string part = body.substr(pos, nextDelim - pos);

        string::size_type headerEnd = part.find("\r\n\r\n");
        if (headerEnd == string::npos) {
            pos = nextDelim + delim.size();
            if (body.compare(nextDelim, endDelim.size(), endDelim) == 0) break;
            continue;
        }

        string headers = part.substr(0, headerEnd);
        string data = part.substr(headerEnd + 4);

        if (data.size() >= 2 && data[data.size() - 2] == '\r' &&
            data[data.size() - 1] == '\n')
            data.erase(data.size() - 2);

        string            filename;
        string::size_type fnPos = headers.find("filename=\"");
        if (fnPos != string::npos) {
            fnPos += 10;
            string::size_type fnEnd = headers.find('"', fnPos);
            if (fnEnd == string::npos) {
                malformed = true;
                return files;
            }
            filename = headers.substr(fnPos, fnEnd - fnPos);
        }

        string            ct;
        string::size_type ctPos = headers.find("Content-Type: ");
        if (ctPos != string::npos) {
            ctPos += 14;
            string::size_type ctEnd = headers.find("\r\n", ctPos);
            ct = (ctEnd != string::npos) ? headers.substr(ctPos, ctEnd - ctPos)
                                         : headers.substr(ctPos);
        }

        if (!filename.empty()) {
            UploadedFile file;
            file.filename = _sanitizeFilename(filename);
            file.contentType = ct;
            file.data = data;
            if (!file.filename.empty()) files.push_back(file);
        }

        pos = nextDelim + delim.size();
        if (body.compare(nextDelim, endDelim.size(), endDelim) == 0) break;
    }

    return files;
}

string PostHttpHandler::_sanitizeFilename(const string& raw) {
    string name = raw;

    string::size_type slash = name.find_last_of("/\\");
    if (slash != string::npos) name = name.substr(slash + 1);

    string safe;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (c == '\0' || c == '/' || c == '\\') continue;
        if (c == '.' && safe == ".") continue;
        safe += c;
    }

    if (safe.empty() || safe == "." || safe == "..") {
        ostringstream fallback;
        fallback << "upload_" << static_cast<unsigned long>(time(NULL));
        return fallback.str();
    }
    return safe;
}

HttpResponse PostHttpHandler::handle(const HttpRequest& request) const {
    std::string cleanUri;
    int         uriErr = 0;
    if (!sanitizeUriPath(request.uri, cleanUri, uriErr))
        return errorResponse(uriErr == 400 ? 400 : 403,
                             uriErr == 400 ? "Bad Request" : "Forbidden",
                             _serverConfig);

    HttpRequest sanitized = request;
    sanitized.uri = cleanUri;

    const LocationConfig* location = findLocation(sanitized.uri, _serverConfig);

    if (location != NULL) {
        const vector<string>& methods = location->getMethods();
        if (!methods.empty()) {
            bool allowed = false;
            for (size_t i = 0; i < methods.size(); ++i) {
                if (methods[i] == "POST") {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) return _methodNotAllowedResponse(methods);
        }
    }

    if (_serverConfig != NULL) {
        size_t maxBody = _serverConfig->getClientMaxBodySize();
        if (maxBody > 0 && request.body.size() > maxBody)
            return errorResponse(413, "Payload Too Large", _serverConfig);
    }

    if (location == NULL || !location->hasUploadDirective() ||
        !location->getUploadEnabled())
        return errorResponse(403, "Forbidden", _serverConfig);

    string uploadDir = location->getUploadPath();
    if (uploadDir.empty())
        return errorResponse(500, "Internal Server Error", _serverConfig);

    struct stat dirStat = {};
    if (stat(uploadDir.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode))
        return errorResponse(500, "Internal Server Error", _serverConfig);

    map<string, string>::const_iterator ctIt =
        request.headers.find("content-type");
    bool   isMultipartRequest = false;
    string boundary;
    if (ctIt != request.headers.end()) {
        const string& ctv = ctIt->second;
        if (ctv.size() >= 19 &&
            ctv.compare(0, 19, "multipart/form-data") == 0) {
            isMultipartRequest = true;
            if (!_extractBoundary(ctv, boundary))
                return errorResponse(400, "Bad Request", _serverConfig);
        }
    }

    if (isMultipartRequest) {
        bool                 malformedMultipart = false;
        vector<UploadedFile> files =
            _parseMultipart(request.body, boundary, malformedMultipart);
        if (malformedMultipart)
            return errorResponse(400, "Bad Request", _serverConfig);
        if (files.empty())
            return errorResponse(400, "Bad Request", _serverConfig);

        ostringstream resultBody;
        for (size_t i = 0; i < files.size(); ++i) {
            string filePath = uploadDir + "/" + files[i].filename;

            ofstream out(filePath.c_str(), ios::out | ios::binary | ios::trunc);
            if (!out.is_open())
                return errorResponse(500, "Internal Server Error",
                                     _serverConfig);

            out.write(files[i].data.c_str(),
                      static_cast<streamsize>(files[i].data.size()));
            if (!out.good()) {
                out.close();
                std::remove(filePath.c_str());
                return errorResponse(500, "Internal Server Error",
                                     _serverConfig);
            }
            out.close();

            if (i > 0) resultBody << "\n";
            resultBody << files[i].filename << " (" << files[i].data.size()
                       << " bytes)";
        }

        HttpResponse response(201, "Created");
        response.setBody(resultBody.str());
        response.setHeader("Content-Type", "text/plain");

        ostringstream cl;
        cl << response.body.size();
        response.setHeader("Content-Length", cl.str());
        response.setHeader("Connection", "close");
        return response;
    }

    string relPath = sanitized.uri.substr(location->getPath().size());
    if (!relPath.empty() && relPath[0] == '/') relPath = relPath.substr(1);

    if (relPath.empty()) {
        ostringstream name;
        name << "upload_" << static_cast<unsigned long>(time(NULL));
        relPath = name.str();
    }

    string filePath = uploadDir + "/" + relPath;

    ofstream outFile(filePath.c_str(), ios::out | ios::binary | ios::trunc);
    if (!outFile.is_open())
        return errorResponse(500, "Internal Server Error", _serverConfig);

    outFile.write(request.body.c_str(),
                  static_cast<streamsize>(request.body.size()));
    if (!outFile.good()) {
        outFile.close();
        std::remove(filePath.c_str());
        return errorResponse(500, "Internal Server Error", _serverConfig);
    }
    outFile.close();

    ostringstream bodyMsg;
    bodyMsg << "File uploaded: " << relPath << " (" << request.body.size()
            << " bytes)";

    HttpResponse response(201, "Created");
    response.setBody(bodyMsg.str());
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Location", sanitized.uri);

    ostringstream cl;
    cl << response.body.size();
    response.setHeader("Content-Length", cl.str());
    response.setHeader("Connection", "close");
    return response;
}
