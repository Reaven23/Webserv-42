#!/usr/bin/env python3
import sys

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()

raise RuntimeError("Intentional CGI crash for webserv test")