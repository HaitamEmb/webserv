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
		bool _upload;
		std::string _upload_path;
		std::string _return_url;
		std::string _cgi_extension;
		std::string _cgi_executor;
		bool _sessions;
		std::vector<std::pair<std::string, std::string> > _cgi_handlers;
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
		void setUpload(bool state);
		void setUploadPath(const std::string &path);
		void setReturnUrl(const std::string &url);
		void setCgiExtension(const std::string &extension);
		void setCgiExecutor(const std::string &executor);
		void setSessions(bool enabled);
		void addCgiHandler(const std::string &extension, const std::string &executor);

		std::string getPath() const;
		std::string getRoot() const;
		std::string getIndex() const;
		std::vector<std::string> getMethods() const;
		bool getAutoIndex() const;
		size_t getMaxBodySize() const;
		bool getUpload() const;
		std::string getUploadPath() const;
		std::string getReturnUrl() const;
		std::string getCgiExtension() const;
		std::string getCgiExecutor() const;
		bool getSessions() const;
		const std::vector<std::pair<std::string, std::string> > &getCgiHandlers() const;

		bool AllowedMethod(const std::string &method) const;
};

#endif