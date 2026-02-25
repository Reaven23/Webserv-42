#pragma once

#include <string>

class LocationConfig {
public:
	LocationConfig(void);
	LocationConfig(const LocationConfig& other);
	~LocationConfig(void);

	LocationConfig& operator=(const LocationConfig& other);

	const std::string& getPath() const;
	void setPath(const std::string& path);

private:
	std::string _path;  // ex. "/", "/upload", ...
	//// methods, ... see later
};
