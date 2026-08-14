#ifndef AUTOINDEX_HPP
#define AUTOINDEX_HPP

#include <string>

class Autoindex {
	public:
		static std::string generateListing(const std::string &dir_path, const std::string &request_uri);
};



#endif