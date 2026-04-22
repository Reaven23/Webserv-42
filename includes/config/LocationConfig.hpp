#pragma once

#include <string>
#include <vector>

class LocationConfig {
public:

	enum AutoindexState {
		AUTOINDEX_UNSET,
		AUTOINDEX_OFF,
		AUTOINDEX_ON
	};

	LocationConfig(void);
	LocationConfig(const LocationConfig& other);
	~LocationConfig(void);

	LocationConfig& operator=(const LocationConfig& other);

	const std::string& getPath() const;
	void setPath(const std::string& path);

	void addMethod(const std::string& method);
	const std::vector<std::string>& getMethods() const;

	const std::string& getRoot() const;
	void setRoot(const std::string& root);

	const std::string& getIndex() const;
	void setIndex(const std::string& index);

	AutoindexState getAutoindexState() const;
	void setAutoindexState(AutoindexState state);

	bool hasRedirect() const;
	const std::string& getRedirectTarget() const;
	int getRedirectCode() const;
	void setRedirect(const std::string& target, int code);

	bool hasUploadDirective() const;
	bool getUploadEnabled() const;
	void setUpload(bool enabled);

	const std::string& getUploadPath() const;
	void setUploadPath(const std::string& path);

	bool hasCgiExtension() const;
	const std::string& getCgiExtension() const;
	void setCgiExtension(const std::string& ext);

	bool hasClientMaxBodySize() const;
	size_t getClientMaxBodySize() const;
	void setClientMaxBodySize(size_t size);

	private:
		std::string _path;  // ex. "/", "/upload", ...
		std::vector<std::string> _methods;  // ex. "GET", "POST", ...
		std::string _root;  // ex. "./www"
		std::string _index;  // ex. "index.html"
		AutoindexState _autoindex;  // ex. AUTOINDEX_UNSET, AUTOINDEX_OFF, AUTOINDEX_ON
		std::string _redirect_target;  // ex. "/new"
		int _redirect_code;  // ex. 301
		bool _upload_specified;
		bool _upload_enabled;
		std::string _upload_path;  // ex. "./uploads"
		std::string _cgi_extension;  // ex. ".py"
		bool _client_max_body_size_set;
		size_t _client_max_body_size;
};
