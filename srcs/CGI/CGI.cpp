#include "../../includes/CGI/CGI.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include "../../includes/config/LocationConfig.hpp"
#include "../../includes/config/ServerConfig.hpp"
#include "../../includes/utils/Logger.hpp"

// Constructor
CGI::CGI(Server *server) : _server(server), _errorCode(0){};

// Destructor
CGI::~CGI(){};

int CGI::getErrorCode() const { return _errorCode; }

const std::string &CGI::getScriptPath() const { return _scriptPath; }

bool CGI::resolvePath(const HttpRequest &request) {
    const ServerConfig &config = _server->getConfig();

    const std::vector<LocationConfig> &locations = config.getLocations();
    const LocationConfig *location = 0;
    size_t bestLen = 0;

    for (std::vector<LocationConfig>::const_iterator it = locations.begin();
         it != locations.end(); ++it) {
        const std::string &locPath = it->getPath();
        if (locPath.empty()) continue;
        if (request.uri.compare(0, locPath.size(), locPath) != 0) continue;
        if (request.uri.size() > locPath.size() &&
            locPath[locPath.size() - 1] != '/' &&
            request.uri[locPath.size()] != '/')
            continue;
        if (locPath.size() > bestLen) {
            location = &(*it);
            bestLen = locPath.size();
        }
    }

    std::string root = config.getRoot();
    std::string relPath = request.uri;

    if (location != 0) {
        if (!location->getRoot().empty()) root = location->getRoot();
        relPath = request.uri.substr(location->getPath().size());
        if (relPath.empty() || relPath[0] != '/') relPath = "/" + relPath;
    }

    _scriptPath = root + relPath;

    struct stat st;
    if (stat(_scriptPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        _errorCode = 404;
        return false;
    }
    if (access(_scriptPath.c_str(), X_OK) != 0) {
        _errorCode = 403;
        return false;
    }
    return true;
}

// Public method
bool CGI::registerPipe() {
    // register pipe read-end to epoll
    epoll_event cgiEvent = {};
    cgiEvent.events = EPOLLIN;
    cgiEvent.data.fd = _pipe[0];

    if (epoll_ctl(_server->getEpollFd(), EPOLL_CTL_ADD, _pipe[0], &cgiEvent)) {
        Logger::error(string("epoll_ctl() (client): ") + strerror(errno));
        return (false);
    }

    // register pipe read-end to server instance
    _server->getCgis().push_back(_pipe[0]);

    return (true);
};
