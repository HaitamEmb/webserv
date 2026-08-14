#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigLoc.hpp"
#include <string>
#include <vector>
#include <map>

class CgiHandler {
	private:
		HttpRequest		_request;
		ConfigLoc		_location;
		std::string        _script_path;
		std::string        _cgi_executor; // E.g., "/usr/bin/python3" or "/usr/bin/php-cgi"

		std::map<std::string, std::string> _env;

		void    _setupEnv();
		char**  _getEnvAsCArray() const;
		void    _freeCArray(char** envp) const;

	public:
		CgiHandler(const HttpRequest& req, const ConfigLoc& loc, const std::string& script_path, const std::string& executor);
		~CgiHandler();

		// Executes script via fork/pipe/execve and constructs an HttpResponse
		HttpResponse execute();
};

#endif