#pragma once

#include <map>
#include <string>
#include <vector>

#include "LocationConfig.hpp"

// Socket bind : IP + port.
struct ListenEntry {
	std::string host;
	int port;
};

class ServerConfig {
	public:
		ServerConfig(void);
		ServerConfig(const ServerConfig& other);
		~ServerConfig(void);

		ServerConfig& operator=(const ServerConfig& other);

		// Listens: several `listen` lines in the same `server { }` append here.
		const std::vector<ListenEntry>& getListens() const;
		void addListen(const std::string& host, int port);

		//// TEMP, for compile (until network uses getListens() / ListenEntry)
		std::string getHost() const;
		int getPort() const;
		//// end TEMP

		// Server name
		const std::string& getServerName() const;
		void setServerName(const std::string& name);

		// Root
		const std::string& getRoot() const;
		void setRoot(const std::string& root);

		// Index
		const std::string& getIndex() const;
		void setIndex(const std::string& index);

		// Body size max
		size_t getClientMaxBodySize() const;
		void setClientMaxBodySize(size_t size);

		// Errors pages (by [error code] -> [error page])
		const std::map<int, std::string>& getErrorPages() const;
		void addErrorPage(int code, const std::string& path);

		// Locations
		std::vector<LocationConfig>& getLocations();
		const std::vector<LocationConfig>& getLocations() const;

		// Pre fill root/index into locations if not set
		void applyLocationDefaults();

	private:
		std::vector<ListenEntry> _listens;
		std::string _server_name; //ex. "nimp.com"
		std::string _root;   // ex. "./www"
		std::string _index;  // ex. "index.html"
		size_t _client_max_body_size;
		std::map<int, std::string> _error_pages;  // code -> path
		std::vector<LocationConfig> _locations;
};
