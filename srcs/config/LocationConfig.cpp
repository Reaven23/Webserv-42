#include "../../includes/config/LocationConfig.hpp"

// Constructors

LocationConfig::LocationConfig(void) : _path(""), _methods(){}

LocationConfig::LocationConfig(const LocationConfig& other) : _path(other._path), _methods(other._methods) {}

LocationConfig::~LocationConfig(void) {}


// Setter

void LocationConfig::setPath(const std::string& path) { _path = path; }

void LocationConfig::addMethod(const std::string& method) { _methods.push_back(method); }


// Getter

const std::string& LocationConfig::getPath() const { return (_path); };

const std::vector<std::string>& LocationConfig::getMethods() const { return _methods; }


// Operator

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
	if (this != &other) {
		_path = other._path;
		_methods = other._methods;
		//// ...
	}
	return (*this);
};