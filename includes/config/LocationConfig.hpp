#pragma once

#include <string>
#include <vector>

class LocationConfig {
public:
	LocationConfig(void);
	LocationConfig(const LocationConfig& other);
	~LocationConfig(void);

	LocationConfig& operator=(const LocationConfig& other);

	const std::string& getPath() const;
	void setPath(const std::string& path);

	void addMethod(const std::string& method);
	const std::vector<std::string>& getMethods() const;

private:
	std::string _path;  // ex. "/", "/upload", ...
	std::vector<std::string> _methods; // "GET", "POST", "DELETE"
};
