#include "../../includes/config/ServerConfig.hpp"

// Constructors

ServerConfig::ServerConfig(void)
    : _host(""),
      _port(0),
      _root(""),
      _index(""),
      _client_max_body_size(0),
      _error_pages(),
      _locations(){};

ServerConfig::ServerConfig(const ServerConfig& other)
    : _host(other._host),
      _port(other._port),
      _root(other._root),
      _index(other._index),
      _client_max_body_size(other._client_max_body_size),
      _error_pages(other._error_pages),
      _locations(other._locations){};

ServerConfig::~ServerConfig(void){};

// Setters

void ServerConfig::setListen(const std::string& host, int port) {
  _host = host;
  _port = port;
};

void ServerConfig::setRoot(const std::string& root) { _root = root; }

void ServerConfig::setIndex(const std::string& index) { _index = index; }

void ServerConfig::setClientMaxBodySize(size_t size) {
  _client_max_body_size = size;
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
  _error_pages[code] = path;
}

// Getters

std::string ServerConfig::getHost() const { return _host; }

int ServerConfig::getPort() const { return _port; }

const std::string& ServerConfig::getRoot() const { return _root; }

const std::string& ServerConfig::getIndex() const { return _index; }

size_t ServerConfig::getClientMaxBodySize() const {
  return _client_max_body_size;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
  return _error_pages;
}

std::vector<LocationConfig>& ServerConfig::getLocations() { return _locations; }

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
  return _locations;
}

// Operator

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
  if (this != &other) {
    _host = other._host;
    _port = other._port;
    _root = other._root;
    _index = other._index;
    _client_max_body_size = other._client_max_body_size;
    _error_pages = other._error_pages;
    _locations = other._locations;
  }
  return (*this);
};
