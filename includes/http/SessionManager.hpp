#pragma once

#include <ctime>
#include <map>
#include <string>

const int SESSION_TIMEOUT = 3600;  // 1 hour

struct Session {
    std::map<std::string, std::string> data;
    time_t                             lastAccess;
};

class SessionManager {
   public:
    // Create a new session, returns its id
    static std::string create();

    // Get a session by id, returns NULL if not found or expired
    static Session* get(std::string const& id);

    // Set a key-value pair in a session
    static void set(std::string const& id, std::string const& key,
                    std::string const& value);

    // Destroy a session
    static void destroy(std::string const& id);

    // Remove expired sessions
    static void cleanup();

   private:
    static std::map<std::string, Session> _sessions;
    static std::string                    _generateId();
};
