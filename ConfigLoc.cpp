#include "ConfigLoc.hpp"
#include <algorithm>

ConfigLoc::ConfigLoc() : _path(""), _index("index.html"), _autoindex(false), _max_bodysize(1000000) {};
ConfigLoc::~ConfigLoc() {};

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

std::string ConfigLoc::getPath() const {return _path;};
std::string ConfigLoc::getRoot() const {return _root;};
std::string ConfigLoc::getIndex() const {return _index;};
std::vector<std::string> ConfigLoc::getMethods() const {return _methods;};
bool ConfigLoc::getAutoIndex() const {return _autoindex;};
size_t ConfigLoc::getMaxBodySize() const {return _max_bodysize;};

bool ConfigLoc::AllowedMethod(const std::string &method) const {
	return std::find(_methods.begin(), _methods.end(), method) != _methods.end();
}