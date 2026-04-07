#include "../../includes/http/HttpRequest.hpp"

#include <cctype>
#include <sstream>

HttpMethod HttpRequest::_parseMethod(const std::string& s) {
    if (s == "GET") return GET;
    if (s == "POST") return POST;
    if (s == "DELETE") return DELETE;
    return UNKNOWN;
}

std::string HttpRequest::_trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.length();

    while (start < end && (s[start] == ' ' || s[start] == '\t' ||
                           s[start] == '\r' || s[start] == '\n'))
        ++start;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' ||
                           s[end - 1] == '\r' || s[end - 1] == '\n'))
        --end;
    return s.substr(start, end - start);
}

std::string HttpRequest::_toLower(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.length(); ++i) {
        out[i] =
            static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

void HttpRequest::_splitUriAndQuery(const std::string& full, std::string& uri,
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

bool HttpRequest::_parseHeaders(const std::string& buffer, size_t headers_start,
                                size_t headers_end, HttpRequest& req) {
    std::string headers_section =
        buffer.substr(headers_start, headers_end - headers_start);

    size_t pos = 0;
    while (pos < headers_section.length()) {
        size_t line_end = headers_section.find("\r\n", pos);
        if (line_end == std::string::npos) line_end = headers_section.length();

        std::string line = headers_section.substr(pos, line_end - pos);
        size_t      colon = line.find(':');
        if (line.empty()) {
            if (line_end >= headers_section.length()) break;
            pos = line_end + 2;
            continue;
        }
        if (colon == std::string::npos) return false;

        std::string name = _toLower(_trim(line.substr(0, colon)));
        std::string value = _trim(line.substr(colon + 1));
        if (name.empty()) return false;
        req.headers[name] = value;

        if (line_end >= headers_section.length()) break;
        pos = line_end + 2;
    }
    return true;
}

bool HttpRequest::_parseContentLength(const std::string& value, size_t& out) {
    std::string v = _trim(value);
    if (v.empty()) return false;

    size_t             n = 0;
    std::istringstream iss(v);
    iss >> n;
    if (!iss || !iss.eof()) return false;
    out = n;
    return true;
}

bool HttpRequest::_hasToken(const std::string& csvValue,
                            const std::string& token) {
    std::string lower = _toLower(csvValue);
    size_t      start = 0;
    while (start < lower.length()) {
        size_t comma = lower.find(',', start);
        if (comma == std::string::npos) comma = lower.length();
        std::string t = _trim(lower.substr(start, comma - start));
        if (t == token) return true;
        start = comma + 1;
    }
    return false;
}

ParseStatus HttpRequest::_parseChunkedBody(const std::string& buffer,
                                           size_t             body_start,
                                           std::string&       outBody,
                                           size_t&            outConsumed) {
    outBody.clear();
    outConsumed = 0;
    size_t pos = body_start;

    while (true) {
        size_t line_end = buffer.find("\r\n", pos);
        if (line_end == std::string::npos) return INCOMPLETE;

        std::string size_line = _trim(buffer.substr(pos, line_end - pos));
        size_t      semi = size_line.find(';');
        if (semi != std::string::npos) size_line = size_line.substr(0, semi);
        size_line = _trim(size_line);
        if (size_line.empty()) return ERROR;

        size_t chunk_size = 0;
        for (size_t i = 0; i < size_line.length(); ++i) {
            char   c = size_line[i];
            size_t d = 0;
            if (c >= '0' && c <= '9')
                d = static_cast<size_t>(c - '0');
            else if (c >= 'a' && c <= 'f')
                d = static_cast<size_t>(10 + c - 'a');
            else if (c >= 'A' && c <= 'F')
                d = static_cast<size_t>(10 + c - 'A');
            else
                return ERROR;
            chunk_size = (chunk_size * 16) + d;
        }

        pos = line_end + 2;

        if (chunk_size == 0) {
            while (true) {
                size_t trailer_end = buffer.find("\r\n", pos);
                if (trailer_end == std::string::npos) return INCOMPLETE;
                if (trailer_end == pos) {
                    outConsumed = trailer_end + 2;
                    return OK;
                }
                std::string trailer = buffer.substr(pos, trailer_end - pos);
                if (trailer.find(':') == std::string::npos) return ERROR;
                pos = trailer_end + 2;
            }
        }

        if (buffer.length() < pos + chunk_size + 2) return INCOMPLETE;
        outBody.append(buffer.substr(pos, chunk_size));
        pos += chunk_size;

        if (buffer[pos] != '\r' || buffer[pos + 1] != '\n') return ERROR;
        pos += 2;
    }
}

ParsedHttpRequest HttpRequest::parse(const std::string& buffer) {
    ParsedHttpRequest result;
    result.request = HttpRequest();
    result.status = INCOMPLETE;
    result.consumed = 0;

    size_t headers_end = buffer.find("\r\n\r\n");
    if (headers_end == std::string::npos) return result;

    size_t line_end = buffer.find("\r\n");
    if (line_end == std::string::npos) return result;

    std::string request_line = buffer.substr(0, line_end);

    std::istringstream iss(request_line);
    std::string        method_str, uri_full, version;
    iss >> method_str >> uri_full >> version;
    std::string extra;
    iss >> extra;

    if (method_str.empty() || uri_full.empty() || version.empty() ||
        !extra.empty()) {
        result.status = ERROR;
        return result;
    }

    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        result.status = ERROR;
        return result;
    }
    if (uri_full.empty() || uri_full[0] != '/') {
        result.status = ERROR;
        return result;
    }

    result.request.method = _parseMethod(method_str);
    _splitUriAndQuery(uri_full, result.request.uri, result.request.queryString);
    result.request.version = version;

    if (!_parseHeaders(buffer, line_end + 2, headers_end, result.request)) {
        result.status = ERROR;
        return result;
    }

    if (result.request.version == "HTTP/1.1" &&
        result.request.headers.find("host") == result.request.headers.end()) {
        result.status = ERROR;
        return result;
    }

    size_t body_start = headers_end + 4;
    std::map<std::string, std::string>::const_iterator teIt =
        result.request.headers.find("transfer-encoding");
    if (teIt != result.request.headers.end()) {
        if (!_hasToken(teIt->second, "chunked")) {
            result.status = ERROR;
            return result;
        }
        std::string unchunked;
        size_t      consumed = 0;
        ParseStatus s =
            _parseChunkedBody(buffer, body_start, unchunked, consumed);
        if (s != OK) {
            result.status = s;
            return result;
        }
        result.request.body = unchunked;
        result.consumed = consumed;
        result.status = OK;
        return result;
    }

    size_t                                             content_length = 0;
    std::map<std::string, std::string>::const_iterator clIt =
        result.request.headers.find("content-length");
    if (clIt != result.request.headers.end()) {
        if (!_parseContentLength(clIt->second, content_length)) {
            result.status = ERROR;
            return result;
        }

        if (buffer.length() < body_start + content_length) {
            result.status = INCOMPLETE;
            return result;
        }

        result.request.body = buffer.substr(body_start, content_length);
        result.consumed = body_start + content_length;
        result.status = OK;
        return result;
    }

    result.request.body.clear();
    result.consumed = body_start;
    result.status = OK;
    return result;
}
