#!/bin/sh
printf 'Content-Type: text/plain\n\n'
printf 'Hello from shell CGI\n'
printf 'method=%s\n' "$REQUEST_METHOD"
