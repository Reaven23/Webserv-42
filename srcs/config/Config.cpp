#include "../../includes/config/Config.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

//// temp (debug)
#include <iostream>

// Constructors

Config::Config(void) : _servers() {}

Config::~Config(void) {}

// Helpers functions

std::string Config::readFile(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        // error
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string Config::trim(const std::string& s) {
    std::string::size_type start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::string::size_type end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Config::stripDirectiveValue(const std::string& rest) {
    std::string res = trim(rest);
    if (!res.empty() && res[res.length() - 1] == ';')
        res = trim(res.substr(0, res.length() - 1));
    return res;
}

// find the next { ... }, return the content, update pos
std::string Config::extractBlock(const std::string& content, std::string::size_type& pos) {
    std::string::size_type open = content.find('{', pos);

    if (open == std::string::npos) return "";

    int brackets = 1;
    std::string::size_type i = open + 1;
    while (i < content.length() && brackets > 0) {
        if (content[i] == '{')
            ++brackets;
        else if (content[i] == '}')
            --brackets;
        ++i;
    }

    std::string block = content.substr(open + 1, i - open - 2);
    pos = i;
    return block;
}

// "location /path { ... }"
void Config::parseLocationBlock(const std::string& block, LocationConfig& loc) {
    std::string::size_type pos = 0;
    while (pos < block.length()) {
        std::string::size_type lineEnd = block.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = block.length();
        std::string line = trim(block.substr(pos, lineEnd - pos));
        pos = lineEnd + 1;

        // skip empty/comment
        if (line.empty() || line[0] == '#') continue;

        if (line.find("upload_path") == 0) {
            std::string rest = stripDirectiveValue(line.substr(11));
            loc.setUploadPath(rest);
            continue;
        }

        if (line.find("upload") == 0) {
            std::string rest = stripDirectiveValue(line.substr(6));
            if (rest == "on")
                loc.setUpload(true);
            else if (rest == "off")
                loc.setUpload(false);
            continue;
        }

        if (line.find("autoindex") == 0) {
            std::string rest = stripDirectiveValue(line.substr(9));
            if (rest == "on")
                loc.setAutoindexState(LocationConfig::AUTOINDEX_ON);
            else if (rest == "off")
                loc.setAutoindexState(LocationConfig::AUTOINDEX_OFF);
            continue;
        }

        if (line.find("redirect") == 0) {
            std::string rest = stripDirectiveValue(line.substr(8));
            std::istringstream iss(rest);
            std::string a;
            std::string b;
            if (!(iss >> a)) continue;
            if (iss >> b) {
                if (a == "301")
                    loc.setRedirect(b, 301);
                else if (a == "302")
                    loc.setRedirect(b, 302);
                else
                    loc.setRedirect(rest, 302);
            } else {
                if (a == "301" || a == "302") continue;
                loc.setRedirect(a, 302);
            }
            continue;
        }

        if (line.find("cgi") == 0) {
            std::string rest = stripDirectiveValue(line.substr(3));
            loc.setCgiExtension(rest);
            continue;
        }

        if (line.find("methods") == 0) {
            std::string rest = stripDirectiveValue(line.substr(7));
            std::string::size_type start = 0;
            while (start < rest.length()) {
                std::string::size_type end = rest.find_first_of(" \t", start);
                if (end == std::string::npos) end = rest.length();
                std::string method = trim(rest.substr(start, end - start));
                if (!method.empty()) loc.addMethod(method);
                start = (end == rest.length()) ? end : end + 1;
            }
            continue;
        }

        if (line.find("root") == 0) {
            std::string rest = stripDirectiveValue(line.substr(4));
            loc.setRoot(rest);
            continue;
        }

        if (line.find("index") == 0) {
            std::string rest = stripDirectiveValue(line.substr(5));
            loc.setIndex(rest);
            continue;
        }
    }
}

void Config::parseServerBlock(const std::string& block) {
    ServerConfig server;

    std::string::size_type pos = 0;

    while (pos < block.length()) {
        std::string::size_type startOfLine = pos;
        std::string::size_type lineEnd = block.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = block.length();
        std::string line = trim(block.substr(pos, lineEnd - pos));
        pos = lineEnd + 1;

        // skip empty/comment
        if (line.empty() || line[0] == '#') continue;

        // listen 8080/127.0.0.1:8080;
        if (line.find("listen") == 0) {
            std::string rest = stripDirectiveValue(line.substr(6));
            std::string host = "0.0.0.0";
            std::string portStr = rest;
            std::string::size_type colon = rest.find(':');
            if (colon != std::string::npos) {
                host = trim(rest.substr(0, colon));
                portStr = trim(rest.substr(colon + 1));
            }
            server.addListen(host, atoi(portStr.c_str()));
            continue;
        }

        if (line.find("client_max_body_size") == 0) {
            std::string rest = stripDirectiveValue(line.substr(20));
            if (!rest.empty()) {
                size_t sz = static_cast<size_t>(strtoul(rest.c_str(), NULL, 10));
                server.setClientMaxBodySize(sz);
            }
            continue;
        }

        if (line.find("error_page") == 0) {
            std::string rest = stripDirectiveValue(line.substr(10));
            std::string::size_type sp = rest.find_first_of(" \t");
            if (sp != std::string::npos) {
                std::string codeStr = trim(rest.substr(0, sp));
                std::string path = trim(rest.substr(sp));
                int code = atoi(codeStr.c_str());
                if (!path.empty()) server.addErrorPage(code, path);
            }
            continue;
        }

        if (line.find("root") == 0) {
            std::string rest = stripDirectiveValue(line.substr(4));
            server.setRoot(rest);
            continue;
        }

        // index index.html;
        if (line.find("index") == 0) {
            std::string rest = stripDirectiveValue(line.substr(5));
            server.setIndex(rest);
            continue;
        }

        // location /path { ... }
        if (line.find("location") == 0) {
            ////std::cout << "location" << std::endl;

            std::string rest = trim(line.substr(8));
            std::string::size_type pathEnd = rest.find_first_of(" \t{");
            std::string locPath = (pathEnd != std::string::npos)
                                      ? trim(rest.substr(0, pathEnd))
                                      : rest;

            /*std::cout << "line :" << line << std::endl;////
            std::cout << "rest :" << rest << std::endl;
            std::cout << "pathEnd :" << pathEnd << std::endl;
            std::cout << "locPath :" << locPath << std::endl;*/

            if (locPath.empty()) locPath = "/";

            std::string::size_type bracePos = block.find('{', startOfLine);
            if (bracePos == std::string::npos) continue;
            std::string inner = extractBlock(block, bracePos);
            pos = bracePos;

            LocationConfig loc;
            loc.setPath(locPath);
            parseLocationBlock(inner, loc);
            server.getLocations().push_back(loc);
            continue;
        }
    }

    _servers.push_back(server);
}

// Main function

void Config::parse(const std::string& path) {
    std::string content = readFile(path);
    if (content.empty()) return;

    ////std::cout << "content: " << content << std::endl;

    std::string::size_type pos = 0;
    while (pos < content.length()) {
        std::string::size_type serverPos = content.find("server", pos);
        if (serverPos == std::string::npos) break;
        pos = serverPos + 6;
        // check if "server" is a whole word:
        // if serverPos > 0, not the first char of the line,
        // if the char before isalnum or '_'
        // if yes, continue (skip)
        if (serverPos > 0 &&
            (isalnum(content[serverPos - 1]) || content[serverPos - 1] == '_'))
            continue;
        std::string block = extractBlock(content, pos);
        if (!block.empty()) {
            ////std::cout << "block: " << block << std::endl;
            parseServerBlock(block);
        }
    }
}

// Getter

std::vector<ServerConfig> const& Config::getServers() const {
    return (_servers);
}

//// Print config, temp (debug)
void Config::print() const {
    for (std::vector<ServerConfig>::size_type i = 0; i < _servers.size(); ++i) {
        const ServerConfig& s = _servers[i];

        std::cout << "--- server " << (i + 1) << " ---" << std::endl;
        const std::vector<ListenEntry>& listens = s.getListens();
        for (std::vector<ListenEntry>::size_type li = 0; li < listens.size();
             ++li) {
            std::cout << "  listen: " << listens[li].host << ":"
                      << listens[li].port << std::endl;
        }
        std::cout << "  root: " << s.getRoot() << std::endl;
        std::cout << "  index: " << s.getIndex() << std::endl;
        std::cout << "  client_max_body_size: " << s.getClientMaxBodySize()
                  << std::endl;
        const std::map<int, std::string>& ep = s.getErrorPages();
        for (std::map<int, std::string>::const_iterator e = ep.begin();
             e != ep.end(); ++e) {
            std::cout << "  error_page " << e->first << " " << e->second
                      << std::endl;
        }

        const std::vector<LocationConfig>& locs = s.getLocations();
        for (std::vector<LocationConfig>::size_type j = 0; j < locs.size();
             ++j) {
            const LocationConfig& loc = locs[j];
            std::cout << "  location '" << loc.getPath() << "'" << std::endl;
            if (!loc.getRoot().empty())
                std::cout << "    root: " << loc.getRoot() << std::endl;
            if (!loc.getIndex().empty())
                std::cout << "    index: " << loc.getIndex() << std::endl;
            if (loc.getAutoindexState() != LocationConfig::AUTOINDEX_UNSET) {
                std::cout << "    autoindex: "
                          << (loc.getAutoindexState() == LocationConfig::AUTOINDEX_ON
                                  ? "on"
                                  : "off")
                          << std::endl;
            }
            if (loc.hasRedirect()) {
                std::cout << "    redirect: " << loc.getRedirectCode() << " -> "
                          << loc.getRedirectTarget() << std::endl;
            }
            if (loc.hasUploadDirective()) {
                std::cout << "    upload: " << (loc.getUploadEnabled() ? "on" : "off")
                          << std::endl;
            }
            if (!loc.getUploadPath().empty())
                std::cout << "    upload_path: " << loc.getUploadPath()
                          << std::endl;
            if (loc.hasCgiExtension())
                std::cout << "    cgi: " << loc.getCgiExtension() << std::endl;
            const std::vector<std::string>& methods = loc.getMethods();
            for (std::vector<std::string>::size_type k = 0; k < methods.size();
                 ++k)
                std::cout << "    method: " << methods[k] << std::endl;
        }
        std::cout << std::endl;
    }
}

// Operator

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _servers = other._servers;
    }
    return *this;
};
