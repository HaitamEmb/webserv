#include "CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <signal.h>

CgiHandler::CgiHandler(const HttpRequest& req, const ConfigLoc& loc, const std::string& script_path, const std::string& executor)
        : _request(req), _location(loc), _script_path(script_path), _cgi_executor(executor),
            _input_fd(-1), _output_fd(-1), _pid(-1), _input_offset(0),
            _output_closed(false), _child_reaped(false), _failed(false) {
    _setupEnv();
}

CgiHandler::~CgiHandler() {
    if (_input_fd >= 0) close(_input_fd);
    if (_output_fd >= 0) close(_output_fd);
    terminate();
}

bool CgiHandler::start() {
    int input_pipe[2];
    int output_pipe[2];
    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        _failed = true;
        return false;
    }
    _pid = fork();
    if (_pid < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        _failed = true;
        return false;
    }
    if (_pid == 0) {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        size_t slash = _script_path.rfind('/');
        std::string script_directory = slash == std::string::npos ? "." : _script_path.substr(0, slash);
        std::string script_name = slash == std::string::npos ? _script_path : _script_path.substr(slash + 1);
        if (chdir(script_directory.c_str()) != 0)
            std::exit(1);
        char **envp = _getEnvAsCArray();
        char *args[3];
        args[0] = const_cast<char *>(_cgi_executor.c_str());
        args[1] = const_cast<char *>(script_name.c_str());
        args[2] = NULL;
        execve(args[0], args, envp);
        _freeCArray(envp);
        std::exit(1);
    }
    close(input_pipe[0]);
    close(output_pipe[1]);
    fcntl(input_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(output_pipe[0], F_SETFL, O_NONBLOCK);
    _input_fd = input_pipe[1];
    _output_fd = output_pipe[0];
    if (_request.getBody().empty()) closeInput();
    return true;
}

void CgiHandler::closeInput() {
    if (_input_fd >= 0) {
        close(_input_fd);
        _input_fd = -1;
    }
}

void CgiHandler::handleInput() {
    const std::string &body = _request.getBody();
    if (_input_fd < 0) return;
    if (_input_offset == body.size()) {
        closeInput();
        return;
    }
    ssize_t written = write(_input_fd, body.data() + _input_offset, body.size() - _input_offset);
    if (written > 0) _input_offset += static_cast<size_t>(written);
    else if (written < 0) _failed = true;
    if (_input_offset == body.size()) closeInput();
}

void CgiHandler::handleOutput() {
    char buffer[4096];
    ssize_t bytes_read = read(_output_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) _raw_output.append(buffer, bytes_read);
    else if (bytes_read == 0) {
        close(_output_fd);
        _output_fd = -1;
        _output_closed = true;
        reap();
    } else {
        _failed = true;
    }
}

bool CgiHandler::isComplete() const { return _output_closed && _child_reaped; }
bool CgiHandler::reap() {
    if (_pid < 0 || _child_reaped) return _child_reaped;
    int status = 0;
    pid_t result = waitpid(_pid, &status, WNOHANG);
    if (result == _pid) {
        _child_reaped = true;
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) _failed = true;
    } else if (result < 0) {
        _child_reaped = true;
        _failed = true;
    }
    return _child_reaped;
}
void CgiHandler::terminate() {
    if (_pid >= 0 && !_child_reaped) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
        _child_reaped = true;
    }
}
bool CgiHandler::hasFailed() const { return _failed; }
int CgiHandler::getInputFd() const { return _input_fd; }
int CgiHandler::getOutputFd() const { return _output_fd; }

HttpResponse CgiHandler::getResponse() const {
    HttpResponse response;
    if (_failed) {
        response.setStatusCode(502);
        response.setBody("<h1>502 Bad Gateway (CGI Execution Failed)</h1>");
        return response;
    }
    size_t header_end = _raw_output.find("\r\n\r\n");
    size_t delimiter_length = 4;
    if (header_end == std::string::npos) {
        header_end = _raw_output.find("\n\n");
        delimiter_length = 2;
    }
    if (header_end == std::string::npos) {
        response.setHeader("Content-Type", "text/html");
        response.setBody(_raw_output);
        return response;
    }
    std::stringstream headers(_raw_output.substr(0, header_end));
    std::string line;
    while (std::getline(headers, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        size_t start = value.find_first_not_of(" \t\r");
        if (start != std::string::npos) value = value.substr(start);
        if (key == "Status") response.setStatusCode(std::atoi(value.c_str()));
        else response.setHeader(key, value);
    }
    response.setBody(_raw_output.substr(header_end + delimiter_length));
    return response;
}

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
