*This project has been created as part of the 42 curriculum by <login>.*

# Description

`webserv` is an HTTP/1.1 server written in C++98. It uses one non-blocking
`poll()` loop for listening sockets and client sockets, parses a small Nginx-
style configuration, serves static files, generates directory listings, and
supports GET, POST uploads, and DELETE.

# Instructions

Build on a POSIX environment with a C++98 compiler:

```sh
make
./webserv [configuration file]
```

When no configuration file is supplied, `default.conf` is used. The sample
server listens on `127.0.0.1:8080` and serves files from `www/`.

Useful checks:

```sh
curl -i http://127.0.0.1:8080/
curl -i -X POST -H 'X-Filename: note.txt' --data 'hello' http://127.0.0.1:8080/uploads
curl -i -X DELETE http://127.0.0.1:8080/uploads/note.txt
```

# Resources

- RFC 9110, HTTP Semantics: https://www.rfc-editor.org/rfc/rfc9110
- RFC 3875, CGI: https://www.rfc-editor.org/rfc/rfc3875
- `poll(2)`, `socket(2)`, and `send(2)` system documentation
- Nginx documentation for configuration and HTTP behavior comparisons

Bonus examples are included in `default.conf`: `/session/` issues a
`WEBSESSID` cookie and counts requests for that session, while `/cgi/` maps
both `.py` files to Python and `.sh` files to `/bin/sh`.

AI assistance was used to inspect the existing implementation, repair the
build and C++ value semantics, identify request-routing and event-loop bugs,
and add focused validation and documentation. All final code decisions and
tests were reviewed in the repository.Architecture overview :

we will have 3 main layers:

-1- I/O handling layer
	doesnt know what HTTP is, just listens and knows if a socket has incoming/outgoing bytes. We use epoll() or poll()

-2- HTTP layer 
	turns raw strings into logical objects
	(request.getMethod())
	we can use classes HTTPREQUEST / HTTPRESPONSE

-3- CGI / Static layer
	call cgi handler or leave it at static file handler.


System design:

-Efficiency:
	non blocking I/O

-Reliability:
	implement a lifecycle for the Client

-Scalability:
	-the use of epoll/poll instead of select ensures a large number of FDs
	however the OS has a limit, run ulimit-n. The time complexity is a bigger bottleneck, using poll() the kernel needs to iterate n times each miliseconds, the solution is using epoll with only an O(1) time complexity.


1-Setting up server:

We will set up a small server running on 8080 port for testing.

to talk with the server, it needs a socket (int fd) that acts as a portal.

we will implement a small HTTP connection with a:
	status line:
		HTTP/1.1 200 ok
	header:
		Content-Type: text/plain
	body:
		Hello World!
we can see the Text Hello World present on local host and everything works fine using a browser or by typping:
	curl -i http://localhost:8080

if we don't follow the rules and try to send a plain string with no status line no header etc, we will get 
an error "Received HTTP/0.9 when not allowed".

2-Config file:

We will be taking the nginx config file as an example.
Our config file will be a simplified version of the nginx config file.

-CGI (Common Gateway Interface):

In short CGI is a protocol for enabling programs or scripts to interact with web servers and web clients.

Normally the browser(client) contacts the HTTP server and demand for the Universal Resource loader ie file name, the Server will parse the URL and look for the filename, if OK sends back the filename otherwise sends error message, finally the client displays the filename or the error. 

Now with the CGI protocol, instead of just displaying a file, the client can ask for execution of a program or script, and the produced result of that execution is sent back to the client to display.

The CGI layer will receive the request and then see if it's a request for script execution or just looking for a file to display like index.html.