#include "../../includes/config/Config.hpp"

// Constructors

Config::Config(void) : _servers(){};

Config::~Config(void){};

// Function

void Config::parse(const std::string& path) {
  //// ...
  (void)path;
}

// Getter

const std::vector<ServerConfig>& Config::getServers() const {
  return (_servers);
}

// Operator

/* Config& Config::operator=(const Config& other) {
        if (this != &other) {
                _servers = other._servers;
        }
        return (*this);
}; */
