#pragma once

#include "ServerConfig.hpp"
#include <string>
#include <vector>

class Config {
public:
	Config(void);
	~Config(void);

	Config& operator=(const Config& other);

	// Parse .conf -> fill _servers
	void parse(const std::string& path);

	// ex: const std::vector<ServerConfig>& servers = config.getServers()
	const std::vector<ServerConfig>& getServers() const;

	// Affiche toute la config (serveurs + locations) sur stdout
	void print() const;

private:
	std::vector<ServerConfig> _servers;

	// read the file, return content
	static std::string readFile(const std::string& path);

	// trim the string
	static std::string trim(const std::string& s);

	// parse a "server { ... }" block and add the result to _servers
	void parseServerBlock(const std::string& block);

	// parse "location /path { ... }", fill the LocationConfig
	static void parseLocationBlock(const std::string& block, LocationConfig& loc);

	// find the next block delimited by { } from pos; update pos after the block
	static std::string extractBlock(const std::string& content, std::string::size_type& pos);
};

