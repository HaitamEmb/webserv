#include "Autoindex.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <iostream>

std::string Autoindex::generateListing(const std::string& dir_path, const std::string& request_uri) 
{
	DIR* dir = opendir(dir_path.c_str());
	if (!dir)
		return "";

	std::string formatted_uri = request_uri;
	if (formatted_uri.empty() || formatted_uri[formatted_uri.length() - 1] != '/')
		formatted_uri += "/";

	std::stringstream html;
	html << "<!DOCTYPE html><html><head><title>Index of " << formatted_uri << "</title>"
		<< "<style>"
		<< "body { font-family: monospace; padding: 20px; background: #1e1e1e; color: #d4d4d4; }"
		<< "h1 { border-bottom: 1px solid #444; padding-bottom: 10px; }"
		<< "ul { list-style: none; padding-left: 0; }"
		<< "li { padding: 6px 0; border-bottom: 1px solid #2d2d2d; }"
		<< "a { color: #569cd6; text-decoration: none; }"
		<< "a:hover { text-decoration: underline; }"
		<< "</style></head><body>"
		<< "<h1>Index of " << formatted_uri << "</h1><ul>";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == ".")
			continue;
		std::string full_item_path = dir_path;
		if (full_item_path[full_item_path.length() - 1] != '/') 
			full_item_path += "/";
		full_item_path += name;

		struct stat st;
		bool is_dir = false;
		if (stat(full_item_path.c_str(), &st) == 0) {
			if (S_ISDIR(st.st_mode))
				is_dir = true;
		}

		std::string link = formatted_uri + name;
		if (is_dir) 
			link += "/";

		html << "<li>"
			<< (is_dir ? "[DIR]  " : "[FILE] ")
			<< "<a href=\"" << link << "\">" << name << (is_dir ? "/" : "") << "</a>"
			<< "</li>";
	}

	closedir(dir);

	html << "</ul><hr><small>Webserv/1.0</small></body></html>";
	return html.str();
}