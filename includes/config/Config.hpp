#pragma once

#include "ServerConfig.hpp"
#include <string>
#include <vector>

class Config {
public:
	Config(void);
	~Config(void);

	// Parse .conf -> fill _servers
	void parse(const std::string& path);

	// ex: const std::vector<ServerConfig>& servers = config.getServers()
	const std::vector<ServerConfig>& getServers() const;

private:
	std::vector<ServerConfig> _servers;
};

