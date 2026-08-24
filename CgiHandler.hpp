#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigLoc.hpp"
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>

class CgiHandler {
	private:
		HttpRequest		_request;
		ConfigLoc		_location;
		std::string        _script_path;
		std::string        _cgi_executor; // E.g., "/usr/bin/python3" or "/usr/bin/php-cgi"

		std::map<std::string, std::string> _env;
		int _input_fd;
		int _output_fd;
		pid_t _pid;
		size_t _input_offset;
		std::string _raw_output;
		bool _output_closed;
		bool _child_reaped;
		bool _failed;

		void    _setupEnv();
		char**  _getEnvAsCArray() const;
		void    _freeCArray(char** envp) const;

	public:
		CgiHandler(const HttpRequest& req, const ConfigLoc& loc, const std::string& script_path, const std::string& executor);
		~CgiHandler();

		bool start();
		void handleInput();
		void handleOutput();
		bool isComplete() const;
		bool reap();
		void terminate();
		bool hasFailed() const;
		int getInputFd() const;
		int getOutputFd() const;
		HttpResponse getResponse() const;
		void closeInput();
};

#endif