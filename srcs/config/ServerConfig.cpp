#include "../../includes/config/ServerConfig.hpp"

// Constructors

ServerConfig::ServerConfig(void)
    : _listens(),
      _root(""),
      _index(""),
      _client_max_body_size(1048576u),  //// 1 MB
      _error_pages(),
      _locations() {}

ServerConfig::ServerConfig(const ServerConfig& other)
    : _listens(other._listens),
      _root(other._root),
      _index(other._index),
      _client_max_body_size(other._client_max_body_size),
      _error_pages(other._error_pages),
      _locations(other._locations) {}

ServerConfig::~ServerConfig(void) {}

// Setters

void ServerConfig::addListen(const std::string& host, int port) {
    ListenEntry toAdd;
    toAdd.host = host;
    toAdd.port = port;
    _listens.push_back(toAdd);
}

void ServerConfig::setRoot(const std::string& root) { _root = root; }

void ServerConfig::setIndex(const std::string& index) { _index = index; }

void ServerConfig::setClientMaxBodySize(size_t size) { _client_max_body_size = size; }

void ServerConfig::addErrorPage(int code, const std::string& path) { _error_pages[code] = path; }

// Getters

const std::vector<ListenEntry>& ServerConfig::getListens() const { return _listens; }

//// TEMP, for compile (until network uses getListens() / ListenEntry)
std::string ServerConfig::getHost() const {
    if (_listens.empty()) return "";
    return _listens[0].host;
}

int ServerConfig::getPort() const {
    if (_listens.empty()) return 0;
    return _listens[0].port;
}

//// end TEMP

const std::string& ServerConfig::getRoot() const { return _root; }

const std::string& ServerConfig::getIndex() const { return _index; }

size_t ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }

const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _error_pages; }

std::vector<LocationConfig>& ServerConfig::getLocations() { return _locations; }

const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }

// Operator

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        _listens = other._listens;
        _root = other._root;
        _index = other._index;
        _client_max_body_size = other._client_max_body_size;
        _error_pages = other._error_pages;
        _locations = other._locations;
    }
    return *this;
}
