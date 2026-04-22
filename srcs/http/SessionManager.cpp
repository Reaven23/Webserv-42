#include "../../includes/http/SessionManager.hpp"

#include <cstdlib>

using namespace std;

map<string, Session> SessionManager::_sessions;

string SessionManager::_generateId() {
    static bool seeded = false;

    if (!seeded) {
        srand(static_cast<unsigned>(time(NULL)));
        seeded = true;
    }

    char const charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    int const  len = 32;
    string     id;

    for (int i = 0; i < len; i++) id += charset[rand() % (sizeof(charset) - 1)];

    return (id);
};

string SessionManager::create() {
    string  id = _generateId();
    Session session;

    session.lastAccess = time(NULL);
    _sessions[id] = session;

    return (id);
}

Session* SessionManager::get(string const& id) {
    map<string, Session>::iterator it = _sessions.find(id);

    if (it == _sessions.end()) return NULL;

    if (time(NULL) - it->second.lastAccess > SESSION_TIMEOUT) {
        _sessions.erase(it);
        return NULL;
    };

    it->second.lastAccess = time(NULL);
    return (&it->second);
}

void SessionManager::set(string const& id, string const& key,
                         string const& value) {
    map<string, Session>::iterator it = _sessions.find(id);

    if (it != _sessions.end()) {
        it->second.data[key] = value;
        it->second.lastAccess = time(NULL);
    }
}

void SessionManager::destroy(string const& id) { _sessions.erase(id); }

void SessionManager::cleanup() {
    time_t                         now = time(NULL);
    map<string, Session>::iterator it = _sessions.begin();

    while (it != _sessions.end()) {
        if (now - it->second.lastAccess > SESSION_TIMEOUT) {
            map<string, Session>::iterator toErase = it;
            it++;
            _sessions.erase(toErase);
        } else {
            ++it;
        }
    }
}
