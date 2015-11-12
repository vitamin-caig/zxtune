#!/usr/bin/env python

import sys
import CGIHTTPServer
import BaseHTTPServer
from SocketServer import ThreadingMixIn

class Handler(CGIHTTPServer.CGIHTTPRequestHandler):
    def translate_path(self, path):
	if path in self.subst:
	    return self.subst[path]
	else:
	    return CGIHTTPServer.CGIHTTPRequestHandler.translate_path(self, path)

class MyServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass

def main():
    port = 8888
    
    Handler.subst = dict()
    for arg in sys.argv[1:]:
	name, val = arg.split('=')
	Handler.subst[name] = val
    httpd = MyServer(("", port), Handler)
    httpd.serve_forever()

if __name__ == "__main__":
    main()
