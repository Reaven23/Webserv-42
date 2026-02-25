#include "../../includes/config/LocationConfig.hpp"

// Constructors

LocationConfig::LocationConfig(void) : _path("") {};

LocationConfig::LocationConfig(const LocationConfig& other) : _path(other._path) {};

LocationConfig::~LocationConfig(void) {};


// Setter

void LocationConfig::setPath(const std::string& path) { _path = path; };


// Getter

const std::string& LocationConfig::getPath() const { return (_path); };


// Operator

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
	if (this != &other) {
		_path = other._path;
		//// ...
	}
	return (*this);
};