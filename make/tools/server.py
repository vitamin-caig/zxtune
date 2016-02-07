#!/usr/bin/env python3

import sys
from http.server import CGIHTTPRequestHandler
from http.server import HTTPServer
from socketserver import ThreadingMixIn

class Handler(CGIHTTPRequestHandler):
    def translate_path(self, path):
        if path in self.subst:
            return self.subst[path]
        else:
            return CGIHTTPRequestHandler.translate_path(self, path)

class MyServer(ThreadingMixIn, HTTPServer):
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
