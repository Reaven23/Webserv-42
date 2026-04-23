#include "../../includes/config/Config.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <set>
#include <sstream>
#include <stdexcept>

//// temp (debug)
#include <iostream>

// Constructors

Config::Config(void) : _servers() {}

Config::~Config(void) {}


// Helpers functions

std::string Config::readFile(const std::string& path) {
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("cannot open config file: " + path);
	std::stringstream ss;
	ss << file.rdbuf();
	std::string content = ss.str();
	if (content.empty())
		throw std::runtime_error("config file is empty: " + path);
	return content;
}

void Config::validate() const {
	if (_servers.empty())
		throw std::runtime_error("no server block in config");

	std::set<std::string> uniqueListens; // set to check for duplicates

	for (size_t serverIndex = 0; serverIndex < _servers.size(); serverIndex++) {
		const std::vector<ListenEntry>& listens = _servers[serverIndex].getListens();
		if (listens.empty())
			throw std::runtime_error("server block has no listen directive");

		for (size_t listenIndex = 0; listenIndex < listens.size(); listenIndex++) {
			int port = listens[listenIndex].port;
			if (port < 1 || port > 65535)
				throw std::runtime_error("invalid listen port");

			std::string host = listens[listenIndex].host.empty() ? "0.0.0.0" : listens[listenIndex].host;

			std::ostringstream oss;
			oss << host << ':' << port;//no to_string so ostringstream

			std::string listenKey = oss.str();
			if (!uniqueListens.insert(listenKey).second)
				throw std::runtime_error("duplicate listen " + listenKey);
		}

		const std::vector<LocationConfig>& locations = _servers[serverIndex].getLocations();
		for (size_t locationIndex = 0; locationIndex < locations.size(); locationIndex++) {
			const LocationConfig& location = locations[locationIndex];
			if (location.getUploadEnabled() && location.getUploadPath().empty())
				throw std::runtime_error("upload is enabled but upload_path is not set for location: " + location.getPath());
		}
	}
}

std::string Config::trim(const std::string& s) {
	std::string::size_type start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) 
		return "";
	std::string::size_type end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::string Config::stripDirectiveValue(const std::string& rest) {
	std::string res = trim(rest);
	if (!res.empty() && res[res.length() - 1] == ';')
		res = trim(res.substr(0, res.length() - 1));
	return res;
}

void Config::enforceDirectiveSemicolon(const std::string& line) {
	std::string s = line;
	std::string::size_type h = s.find('#'); // skip end line comment
	if (h != std::string::npos)
		s = trim(s.substr(0, h));
	if (s.empty())
		return;
	char c = s[s.length() - 1];
	if (c == '{' || c == '}')
		return;
	if (c != ';')
		throw std::runtime_error("expected ';' at end of directive: " + line);
}

// find the next { ... }, return the content, update pos
std::string Config::extractBlock(const std::string& content, std::string::size_type& pos) {
	std::string::size_type open = content.find('{', pos);

	if (open == std::string::npos)
		throw std::runtime_error("missing opening brace");

	int brackets = 1;
	std::string::size_type i = open + 1;
	while (i < content.length() && brackets > 0) {
		if (content[i] == '{')
			++brackets;
		else if (content[i] == '}')
			--brackets;
		++i;
	}
	if (brackets > 0)
		throw std::runtime_error("missing a bracket");

	std::string block = content.substr(open + 1, i - open - 2);
	pos = i;
	return block;
}

std::string::size_type Config::nextLocationLineIndex(const std::string& block, std::string::size_type from) {
	std::string::size_type at = from;
	while (at < block.length()) {
		std::string::size_type lineEnd = block.find('\n', at);
		if (lineEnd == std::string::npos) 
			return std::string::npos;
		std::string line = trim(block.substr(at, lineEnd - at));
		if (!line.empty() && line.find("location") == 0)
			return at;
		at = lineEnd + 1;
		if (lineEnd == block.length()) 
			break;
	}
	return std::string::npos;
}

static const std::string validCgiExtensions[] = { ".py", ".js", ".bla" };

static bool isValidCgiExtension(const std::string& ext) {
	for (std::string::size_type i = 0; i < sizeof(validCgiExtensions) / sizeof(validCgiExtensions[0]); i++) {
		if (ext == validCgiExtensions[i])
			return true;
	}
	return false;
}

static bool isValidErrorCode(const int code) {
	return code >= 300 && code <= 599;
}

static bool isValidRedirectCode(const std::string& code) {
	return code == "301" || code == "302";
}

static bool isValidMethod(const std::string& method) {
	return !method.empty() && (method == "GET" || method == "POST" || method == "DELETE");
}

static bool isValidRedirectTarget(const std::string& target) {
	return !target.empty() && (target[0] == '/' || !target.find("http://") || !target.find("https://"));
}

static bool isIdentifierContinuationChar(unsigned char c) {
	return (std::isalnum(c) != 0) || c == '_';
}

// "location /path { ... }"
void Config::parseLocationBlock(const std::string& block, LocationConfig& loc) {
	std::string::size_type pos = 0;
	while (pos < block.length()) {
		std::string::size_type lineEnd = block.find('\n', pos);
		if (lineEnd == std::string::npos) 
			lineEnd = block.length();
		std::string line = trim(block.substr(pos, lineEnd - pos));
		pos = lineEnd + 1;

		// skip empty/comment
		if (line.empty() || line[0] == '#') continue;

		enforceDirectiveSemicolon(line);

		if (line.find("upload_path") == 0) {
			std::string rest = stripDirectiveValue(line.substr(11));
			if (rest.empty())
				throw std::runtime_error("missing upload_path: " + line);
			loc.setUploadPath(rest);
			continue;
		}

		if (line.find("upload") == 0) {
			std::string rest = stripDirectiveValue(line.substr(6));
			if (rest == "on")
				loc.setUpload(true);
			else if (rest == "off")
				loc.setUpload(false);
			else
				throw std::runtime_error("invalid upload directive: " + line);
			continue;
		}

		if (line.find("autoindex") == 0) {
			std::string rest = stripDirectiveValue(line.substr(9));
			if (rest == "on")
				loc.setAutoindexState(LocationConfig::AUTOINDEX_ON);
			else if (rest == "off")
				loc.setAutoindexState(LocationConfig::AUTOINDEX_OFF);
			else
				throw std::runtime_error("invalid autoindex directive: " + line);
			continue;
		}

		if (line.find("redirect") == 0) {
			std::string rest = stripDirectiveValue(line.substr(8));
			std::istringstream iss(rest);
			std::string tok1;
			std::string tok2;
			bool twoTokens = false;

			if (!(iss >> tok1))
				throw std::runtime_error("missing redirect code or target");
			if (iss >> tok2)
				twoTokens = true;
			if (twoTokens) {
				if (isValidRedirectCode(tok1) && isValidRedirectTarget(tok2))
					loc.setRedirect(tok2, atoi(tok1.c_str()));
				else
					throw std::runtime_error("invalid redirect code or target: " + tok1 + " " + tok2);
			} else {
				if (!isValidRedirectTarget(tok1))
					throw std::runtime_error("invalid redirect target: " + tok1);
				loc.setRedirect(tok1, 302);
			}
			continue;
		}

		if (line.find("cgi") == 0) {
			std::string rest = stripDirectiveValue(line.substr(3));

			if (rest.empty())
				throw std::runtime_error("missing cgi extension: " + line);
			if (!isValidCgiExtension(rest))
				throw std::runtime_error("invalid cgi extension: " + rest);

			loc.setCgiExtension(rest);
			continue;
		}

		if (line.find("methods") == 0) {
			std::string rest = stripDirectiveValue(line.substr(7));
			std::string::size_type start = 0;
			if (rest.empty())
				throw std::runtime_error("missing methods: " + line);
			while (start < rest.length()) {
				std::string::size_type end = rest.find_first_of(" \t", start);
				if (end == std::string::npos) 
					end = rest.length();
				std::string method = trim(rest.substr(start, end - start));
				if (!isValidMethod(method))
					throw std::runtime_error("invalid method: " + method);
				loc.addMethod(method);
				start = (end == rest.length()) ? end : end + 1;
			}
			continue;
		}

		if (line.find("root") == 0) {
			std::string rest = stripDirectiveValue(line.substr(4));
			if (rest.empty())
				throw std::runtime_error("missing root: " + line);
			loc.setRoot(rest);
			continue;
		}

		if (line.find("index") == 0) {
			std::string rest = stripDirectiveValue(line.substr(5));
			if (rest.empty())
				throw std::runtime_error("missing index: " + line);
			loc.setIndex(rest);
			continue;
		}

		if (line.find("client_max_body_size") == 0) {
			std::string rest = stripDirectiveValue(line.substr(20));
			if (!rest.empty()) {
				size_t sz = static_cast<size_t>(strtoul(rest.c_str(), NULL, 10));
				loc.setClientMaxBodySize(sz);
			}
			else 
				throw std::runtime_error("missing client_max_body_size: " + line);
			continue;
		}

		throw std::runtime_error("unknown directive in location block: " + line);
	}
}

void Config::parseServerBlock(const std::string& block) {
	ServerConfig server;

	std::string::size_type pos = 0;

	while (pos < block.length()) {
		std::string::size_type startOfLine = pos;
		std::string::size_type lineEnd = block.find('\n', pos);

		if (lineEnd == std::string::npos) 
			lineEnd = block.length();

		std::string line = trim(block.substr(pos, lineEnd - pos));
		pos = lineEnd + 1; // next line pos

		// skip empty/comment
		if (line.empty() || line[0] == '#') 
			continue;

		// if not location (for ; check)
		if (line.find("location") != 0)
			enforceDirectiveSemicolon(line);

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

		if (line.find("server_name") == 0) {
			std::string rest = stripDirectiveValue(line.substr(11));
			if (rest.empty())
				throw std::runtime_error("missing server_name: " + line);
			server.setServerName(rest);
			continue;
		}

		if (line.find("client_max_body_size") == 0) {
			std::string rest = stripDirectiveValue(line.substr(20));
			if (!rest.empty()) {
				size_t sz = static_cast<size_t>(strtoul(rest.c_str(), NULL, 10));// strtoul not atoi cause client_max_body_size can be > INT_MAX
				server.setClientMaxBodySize(sz);
			}
			else 
				throw std::runtime_error("missing client_max_body_size: " + line);
			continue;
		}

		if (line.find("error_page") == 0) {
			std::string rest = stripDirectiveValue(line.substr(10));
			std::string::size_type sp = rest.find_first_of(" \t");

			if (sp == std::string::npos || rest.empty())
				throw std::runtime_error("missing path for error page: " + line);

			std::string codeStr = trim(rest.substr(0, sp));
			std::string path = trim(rest.substr(sp));
			if (codeStr.empty() || path.empty())
				throw std::runtime_error("invalid error page directive: " + line);
		
			int code = atoi(codeStr.c_str());
			if (!isValidErrorCode(code))
				throw std::runtime_error("invalid error code: " + codeStr);
			server.addErrorPage(code, path);
			continue;
		}

		if (line.find("root") == 0) {
			std::string rest = stripDirectiveValue(line.substr(4));
			if (rest.empty())
				throw std::runtime_error("missing root: " + line);
			server.setRoot(rest);
			continue;
		}

		// index index.html;
		if (line.find("index") == 0) {
			std::string rest = stripDirectiveValue(line.substr(5));
			if (rest.empty())
				throw std::runtime_error("missing index: " + line);
			server.setIndex(rest);
			continue;
		}

		// location /path { ... }
		if (line.find("location") == 0) {
			std::string rest = trim(line.substr(8));
			if (rest.empty())
				throw std::runtime_error("missing location: " + line);
			
			std::string::size_type pathEnd = rest.find_first_of(" \t{");
			std::string locPath = (pathEnd != std::string::npos) ? trim(rest.substr(0, pathEnd)) : rest;

			if (locPath.empty())
				locPath = "/";

			std::string::size_type bracePos = block.find('{', startOfLine);
			std::string::size_type nextLocLine = nextLocationLineIndex(block, pos);
			if (bracePos == std::string::npos || (nextLocLine != std::string::npos && bracePos >= nextLocLine))
				throw std::runtime_error("location block has no opening brace: " + line);
			
			std::string inner = extractBlock(block, bracePos);
			pos = bracePos;

			LocationConfig loc;
			const std::vector<LocationConfig>& existingLocs = server.getLocations();
			for (size_t j = 0; j < existingLocs.size(); j++) {
				if (existingLocs[j].getPath() == locPath)
					throw std::runtime_error("duplicate location path: " + locPath);
			}
			loc.setPath(locPath);
			parseLocationBlock(inner, loc);
			server.getLocations().push_back(loc);
			continue;
		}

		throw std::runtime_error("unknown directive in server block: " + line);
	}

	server.applyLocationDefaults();
	_servers.push_back(server);
}


// Main function

void Config::parse(const std::string& path) {
	std::string content = readFile(path);
	std::string::size_type pos = 0;

	while (pos < content.length()) {
		std::string::size_type serverPos = content.find("server", pos);
		if (serverPos == std::string::npos)
			break;

		// "server" as substring of another word (e.g. "/myserver", "xserver")
		if (serverPos > 0 && isIdentifierContinuationChar(static_cast<unsigned char>(content[serverPos - 1]))) {
			pos = serverPos + 6;
			continue;
		}

		// Before testing keyword form, skip matches inside a # comment line
		std::string::size_type lineStart = content.rfind('\n', serverPos);
		if (lineStart == std::string::npos)
			lineStart = 0;
		else
			lineStart += 1;
		std::string::size_type hashPos = content.find('#', lineStart);
		if (hashPos != std::string::npos && hashPos < serverPos) {
			pos = serverPos + 6;
			continue;
		}

		std::string::size_type afterWord = serverPos + 6;

		// if afterWord is out of bounds
		if (afterWord >= content.length())
			throw std::runtime_error("unexpected end of file after 'server'");

		// "server {" or "server\n{" NOT "serverL {"
		unsigned char after = static_cast<unsigned char>(content[afterWord]);
		if (!std::isspace(after) && after != '{')
			throw std::runtime_error("expected whitespace or '{' after 'server'");

		pos = afterWord;
		std::string block = extractBlock(content, pos);

		if (trim(block).empty())
			throw std::runtime_error("empty server block");
		parseServerBlock(block);
	}

	validate();
}


// Getter

std::vector<ServerConfig> const& Config::getServers() const {
	return (_servers);
}

//// Print config, temp (debug), to remove
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
		std::cout << "  server_name: " << s.getServerName() << std::endl;
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
