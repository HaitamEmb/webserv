#include "CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

CgiHandler::CgiHandler(const HttpRequest& req, const ConfigLoc& loc, const std::string& script_path, const std::string& executor)
    : _request(req), _location(loc), _script_path(script_path), _cgi_executor(executor) {
    _setupEnv();
}

CgiHandler::~CgiHandler() {}

void CgiHandler::_setupEnv() {
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_PROTOCOL"]    = "HTTP/1.1";
    _env["REQUEST_METHOD"]     = _request.getMethod();
    _env["SCRIPT_FILENAME"]    = _script_path;
    _env["PATH_INFO"]          = _request.getPath();
    _env["QUERY_STRING"]       = _request.getQueryString(); // Ensure HttpRequest parses query string after '?'

    // Content headers for POST
    _env["CONTENT_TYPE"]   = _request.getHeader("Content-Type");
    _env["CONTENT_LENGTH"] = _request.getHeader("Content-Length");

    // Pass Cookie header if present
    std::string cookie = _request.getHeader("Cookie");
    if (!cookie.empty()) {
        _env["HTTP_COOKIE"] = cookie;
    }

    // Pass User-Agent header if present
    std::string ua = _request.getHeader("User-Agent");
    if (!ua.empty()) {
        _env["HTTP_USER_AGENT"] = ua;
    }
}

char** CgiHandler::_getEnvAsCArray() const {
    char** envp = new char*[_env.size() + 1];
    size_t i = 0;

    std::map<std::string, std::string>::const_iterator it;
    for (it = _env.begin(); it != _env.end(); ++it) {
        std::string env_str = it->first + "=" + it->second;
        envp[i] = new char[env_str.size() + 1];
        std::strcpy(envp[i], env_str.c_str());
        ++i;
    }
    envp[i] = NULL;
    return envp;
}

void CgiHandler::_freeCArray(char** envp) const {
    if (!envp) return;
    for (size_t i = 0; envp[i] != NULL; ++i) {
        delete[] envp[i];
    }
    delete[] envp;
}

HttpResponse CgiHandler::execute() {
    HttpResponse res;

    int input_pipe[2];   // Parent writes request body -> Child reads standard input
    int output_pipe[2];  // Child writes CGI output -> Parent reads response

    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        res.setStatusCode(500);
        res.setBody("<h1>500 Internal Server Error (Pipe Failed)</h1>");
        return res;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        res.setStatusCode(500);
        res.setBody("<h1>500 Internal Server Error (Fork Failed)</h1>");
        return res;
    }

    // ==========================================
    // CHILD PROCESS
    // ==========================================
    if (pid == 0) {
        // Redirect standard input to read end of input_pipe
        dup2(input_pipe[0], STDIN_FILENO);
        // Redirect standard output to write end of output_pipe
        dup2(output_pipe[1], STDOUT_FILENO);

        // Close unused pipe ends
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);

        char** envp = _getEnvAsCArray();

        // Prepare arguments: executor, script_path, NULL
        char* args[3];
        args[0] = const_cast<char*>(_cgi_executor.c_str());
        args[1] = const_cast<char*>(_script_path.c_str());
        args[2] = NULL;

        execve(args[0], args, envp);

        // If execve returns, it failed
        _freeCArray(envp);
        std::exit(1);
    }

    // PARENT PROCESS
    close(input_pipe[0]);  // Parent doesn't read from input pipe
    close(output_pipe[1]); // Parent doesn't write to output pipe

    const std::string& body = _request.getBody();
    if (!body.empty()) {
        write(input_pipe[1], body.c_str(), body.size());
    }
    close(input_pipe[1]);

    std::string raw_cgi_output;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(output_pipe[0], buffer, sizeof(buffer))) > 0) {
        raw_cgi_output.append(buffer, bytes_read);
    }
    close(output_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        res.setStatusCode(502);
        res.setBody("<h1>502 Bad Gateway (CGI Execution Failed)</h1>");
        return res;
    }

    size_t header_end = raw_cgi_output.find("\r\n\r\n");
    size_t delim_len = 4;
    if (header_end == std::string::npos) {
        header_end = raw_cgi_output.find("\n\n");
        delim_len = 2;
    }

    res.setStatusCode(200);

    if (header_end != std::string::npos) {
        std::string headers_part = raw_cgi_output.substr(0, header_end);
        std::string body_part = raw_cgi_output.substr(header_end + delim_len);

        std::stringstream ss(headers_part);
        std::string header_line;
        while (std::getline(ss, header_line)) {
            size_t colon = header_line.find(':');
            if (colon != std::string::npos) {
                std::string key = header_line.substr(0, colon);
                std::string value = header_line.substr(colon + 1);
                // Trim leading spaces from value
                size_t start = value.find_first_not_of(" \t\r");
                if (start != std::string::npos) value = value.substr(start);
                // Trim trailing carriage returns
                size_t end = value.find_last_not_of("\r");
                if (end != std::string::npos) value = value.substr(0, end + 1);

                if (key == "Status") {
                    res.setStatusCode(std::atoi(value.c_str()));
                } else {
                    res.setHeader(key, value);
                }
            }
        }
        res.setBody(body_part);
    } else {
        res.setHeader("Content-Type", "text/html");
        res.setBody(raw_cgi_output);
    }

    return res;
}