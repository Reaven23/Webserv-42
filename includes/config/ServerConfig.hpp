#pragma once

#include "LocationConfig.hpp"
#include <map>
#include <string>
#include <vector>

class ServerConfig {
public:
	ServerConfig(void);
	ServerConfig(const ServerConfig& other);
	~ServerConfig(void);

	ServerConfig& operator=(const ServerConfig& other);

	// Listen
	std::string getHost() const;
	int getPort() const;
	void setListen(const std::string& host, int port);

	// Root
	const std::string& getRoot() const;
	void setRoot(const std::string& root);

	// Index
	const std::string& getIndex() const;
	void setIndex(const std::string& index);

	// Body size max
	size_t getClientMaxBodySize() const;
	void setClientMaxBodySize(size_t size);

	// Errors pages (by [error code] -> [error paage])
	const std::map<int, std::string>& getErrorPages() const;
	void addErrorPage(int code, const std::string& path);

	// Locations
	std::vector<LocationConfig>& getLocations(); // mutable (ex: serverConfig.getLocations().push_back(...))
	const std::vector<LocationConfig>& getLocations() const; // const (ex: const std::vector<LocationConfig>& locations = serverConfig.getLocations())

private:
	std::string _host;   // ex. "127.0.0.1"
	int _port;           // ex. 8080
	std::string _root;   // ex. "./www"
	std::string _index;  // ex. "index.html"
	size_t _client_max_body_size;
	std::map<int, std::string> _error_pages;  // code -> path
	std::vector<LocationConfig> _locations;
};