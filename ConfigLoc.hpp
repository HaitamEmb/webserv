#ifndef CONFIGLOC_HPP
#define CONFIGLOC_HPP

#include <vector>
#include <string>
#include <iostream>

class ConfigLoc
{
	private:
		std::string _root;
		std::string _path;
		std::string _index;
		bool _autoindex;
		size_t _max_bodysize;
		std::vector<std::string> _methods;

	public:
		ConfigLoc();
		~ConfigLoc();
		ConfigLoc(const ConfigLoc &other);
		ConfigLoc &operator=(const ConfigLoc &other);
		void setPath(const std::string &path);
		void setRoot(const std::string &root);
		void setIndex(const std::string &index);
		void addMethod(const std::string &method);
		void setMaxBodySize(size_t size);
		void setAutoIndex(bool state);

		std::string getPath() const;
		std::string getRoot() const;
		std::string getIndex() const;
		std::vector<std::string> getMethods() const;
		bool getAutoIndex() const;
		size_t getMaxBodySize() const;

		bool AllowedMethod(const std::string &method) const;
};

#endif