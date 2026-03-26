#include "../../includes/config/LocationConfig.hpp"

// Constructors

LocationConfig::LocationConfig(void)
    : _path(""),
      _methods(),
      _root(""),
      _index(""),
      _autoindex(AUTOINDEX_UNSET),
      _redirect_target(""),
      _redirect_code(0),
      _upload_specified(false),
      _upload_enabled(false),
      _upload_path(""),
      _cgi_extension("") {}

LocationConfig::LocationConfig(const LocationConfig& other)
    : _path(other._path),
      _methods(other._methods),
      _root(other._root),
      _index(other._index),
      _autoindex(other._autoindex),
      _redirect_target(other._redirect_target),
      _redirect_code(other._redirect_code),
      _upload_specified(other._upload_specified),
      _upload_enabled(other._upload_enabled),
      _upload_path(other._upload_path),
      _cgi_extension(other._cgi_extension) {}

LocationConfig::~LocationConfig(void) {}


// Setters

void LocationConfig::setPath(const std::string& path) { _path = path; }

void LocationConfig::addMethod(const std::string& method) { _methods.push_back(method); }

void LocationConfig::setRoot(const std::string& root) { _root = root; }

void LocationConfig::setIndex(const std::string& index) { _index = index; }

void LocationConfig::setAutoindexState(AutoindexState state) { _autoindex = state; }

void LocationConfig::setRedirect(const std::string& target, int code) {
    _redirect_target = target;
    _redirect_code = code;
}

void LocationConfig::setUpload(bool enabled) {
    _upload_specified = true;
    _upload_enabled = enabled;
}

void LocationConfig::setUploadPath(const std::string& path) { _upload_path = path; }

void LocationConfig::setCgiExtension(const std::string& ext) { _cgi_extension = ext; }


// Getters

const std::string& LocationConfig::getPath() const { return _path; }

const std::vector<std::string>& LocationConfig::getMethods() const { return _methods; }

const std::string& LocationConfig::getRoot() const { return _root; }

const std::string& LocationConfig::getIndex() const { return _index; }

LocationConfig::AutoindexState LocationConfig::getAutoindexState() const { return _autoindex; }

bool LocationConfig::hasRedirect() const { return !_redirect_target.empty(); }

bool LocationConfig::hasUploadDirective() const { return _upload_specified; }

bool LocationConfig::getUploadEnabled() const { return _upload_enabled; }

const std::string& LocationConfig::getRedirectTarget() const { return _redirect_target; }

int LocationConfig::getRedirectCode() const { return _redirect_code; }

const std::string& LocationConfig::getUploadPath() const { return _upload_path; }

bool LocationConfig::hasCgiExtension() const { return !_cgi_extension.empty(); }

const std::string& LocationConfig::getCgiExtension() const { return _cgi_extension; }


// Operator

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        _path = other._path;
        _methods = other._methods;
        _root = other._root;
        _index = other._index;
        _autoindex = other._autoindex;
        _redirect_target = other._redirect_target;
        _redirect_code = other._redirect_code;
        _upload_specified = other._upload_specified;
        _upload_enabled = other._upload_enabled;
        _upload_path = other._upload_path;
        _cgi_extension = other._cgi_extension;
    }
    return *this;
}
