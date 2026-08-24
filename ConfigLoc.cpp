#include "ConfigLoc.hpp"
#include <algorithm>

ConfigLoc::ConfigLoc() : _path(""), _index("index.html"), _autoindex(false), _max_bodysize(1000000), _upload(false), _sessions(false) {};
ConfigLoc::~ConfigLoc() {};

ConfigLoc::ConfigLoc(const ConfigLoc &other)
{
	*this = other;
}

ConfigLoc &ConfigLoc::operator=(const ConfigLoc &other)
{
	if (this != &other)
	{
		this->_autoindex = other._autoindex;
		this->_index = other._index;
		this->_max_bodysize = other._max_bodysize;
		this->_upload = other._upload;
		this->_upload_path = other._upload_path;
		this->_return_url = other._return_url;
		this->_cgi_extension = other._cgi_extension;
		this->_cgi_executor = other._cgi_executor;
		this->_sessions = other._sessions;
		this->_cgi_handlers = other._cgi_handlers;
		this->_return_url = other._return_url;
		this->_cgi_extension = other._cgi_extension;
		this->_cgi_executor = other._cgi_executor;
		this->_methods = other._methods;
		this->_path = other._path;
		this->_root = other._root;
	}
	return *this;
}

void ConfigLoc::setPath(const std::string &path) {
	_path = path;
}
void ConfigLoc::setRoot(const std::string &root) {
	_root = root;
}
void ConfigLoc::setIndex(const std::string &index) {
	_index = index;
}
void ConfigLoc::setAutoIndex(bool state) {
	_autoindex = state;
}
void ConfigLoc::setMaxBodySize(size_t size) {
	_max_bodysize = size;
}
void ConfigLoc::setUpload(bool state) { _upload = state; }
void ConfigLoc::setUploadPath(const std::string &path) { _upload_path = path; }
void ConfigLoc::setReturnUrl(const std::string &url) { _return_url = url; }
void ConfigLoc::setCgiExtension(const std::string &extension) { _cgi_extension = extension; }
void ConfigLoc::setCgiExecutor(const std::string &executor) { _cgi_executor = executor; }
void ConfigLoc::setSessions(bool enabled) { _sessions = enabled; }
void ConfigLoc::addCgiHandler(const std::string &extension, const std::string &executor) {
	_cgi_handlers.push_back(std::make_pair(extension, executor));
}

void ConfigLoc::addMethod(const std::string &method){
	_methods.push_back(method);
}

std::string ConfigLoc::getPath() const {return _path;};
std::string ConfigLoc::getRoot() const {return _root;};
std::string ConfigLoc::getIndex() const {return _index;};
std::vector<std::string> ConfigLoc::getMethods() const {return _methods;};
bool ConfigLoc::getAutoIndex() const {return _autoindex;};
size_t ConfigLoc::getMaxBodySize() const {return _max_bodysize;};
bool ConfigLoc::getUpload() const {return _upload;};
std::string ConfigLoc::getUploadPath() const {return _upload_path;};
std::string ConfigLoc::getReturnUrl() const {return _return_url;};
std::string ConfigLoc::getCgiExtension() const {return _cgi_extension;};
std::string ConfigLoc::getCgiExecutor() const {return _cgi_executor;};
bool ConfigLoc::getSessions() const {return _sessions;};
const std::vector<std::pair<std::string, std::string> > &ConfigLoc::getCgiHandlers() const {return _cgi_handlers;};

bool ConfigLoc::AllowedMethod(const std::string &method) const {
	return std::find(_methods.begin(), _methods.end(), method) != _methods.end();
}