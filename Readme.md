Architecture overview :

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


2-Config file:

We will be taking the nginx config file as an example.
Our config file will be a simplified version of the nginx config file.

