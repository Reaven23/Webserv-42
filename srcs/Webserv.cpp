#include "../includes/Webserv.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#include "../includes/CGI/CGI.hpp"
#include "../includes/http/SessionManager.hpp"
#include "../includes/network/Client.hpp"
#include "../includes/network/helpers.hpp"
#include "../includes/utils/Logger.hpp"
#include "../includes/utils/Signals.hpp"

using namespace std;

// Constructors
Webserv::Webserv(Config const& config) : _config(config), _epollFd(-1) {}

// Destructor
Webserv::~Webserv() {
    vector<Server*>::const_iterator it;

    for (it = _servers.begin(); it != _servers.end(); it++) {
        delete *it;
    }

    close(_epollFd);
};

// Getters
vector<Server*> const& Webserv::getServers() const { return (_servers); };

// Private methods
void Webserv::_runEventLoop() const {
    epoll_event eventQueue[MAX_EVENTS];

    while (!g_stop) {
        int nbEvents =
            epoll_wait(_epollFd, eventQueue, MAX_EVENTS, EPOLL_BLCK_TIME);
        if (nbEvents == -1) {
            if (g_stop) break;
            Logger::error(string("epoll_wait(): ") + strerror(errno));
            break;
        }

        // Close idle connections
        vector<Server*>::const_iterator itServer;
        for (itServer = _servers.begin(); itServer != _servers.end();
             itServer++) {
            (*itServer)->closeIdleConnections();
        }

        // Close expired sessions
        SessionManager::cleanup();

        // For each event, identify the server responsible for handling it
        for (int i = 0; i < nbEvents; i++) {
            int      fd = eventQueue[i].data.fd;
            uint32_t events = eventQueue[i].events;

            for (itServer = _servers.begin(); itServer != _servers.end();
                 itServer++) {
                // Check for new client event
                if (fd == (*itServer)->getFd()) {
                    (*itServer)->handleNewClient();
                    break;
                }

                // Check for request or response events
                const map<int, Client*>& clientsMap = (*itServer)->getClients();
                map<int, Client*>::const_iterator itClient =
                    clientsMap.find(fd);

                if (itClient != clientsMap.end()) {
                    if (events & EPOLLIN)
                        (*itServer)->handleRequest(fd);
                    else if (events & EPOLLOUT)
                        (*itServer)->handleResponse(fd);
                    else if (events & (EPOLLHUP | EPOLLERR))
                        (*itServer)->removeClient(fd);
                    break;
                }
                // Check for CGI events
                bool cgiHandled = false;
                for (itClient = clientsMap.begin();
                     itClient != clientsMap.end(); ++itClient) {
                    map<int, CGI*>& cgisLookup =
                        itClient->second->getCgisLookup();
                    if (cgisLookup.find(fd) != cgisLookup.end()) {
                        (*itServer)->handleCGI(itClient->first, fd);
                        cgiHandled = true;
                        break;
                    }
                }
                if (cgiHandled) break;
            }
        }
    }
};

// Public methods
void Webserv::start() {
    _epollFd = epoll_create(1);

    if (_epollFd == -1) {
        throw runtime_error(string("epollCreate(): Fatal error: ") +
                            strerror(errno));
    }

    if (!setCloseOnExec(_epollFd)) {
        throw runtime_error(string("fcntl(): Fatal error: ") + strerror(errno));
    }

    vector<ServerConfig> const&          serversConfig = _config.getServers();
    vector<ServerConfig>::const_iterator it;

    for (it = serversConfig.begin(); it != serversConfig.end(); it++) {
        Server* server = new Server(_epollFd, *it);

        _servers.push_back(server);

        // Register server to epoll
        server->setup();
    }

    _runEventLoop();
};
