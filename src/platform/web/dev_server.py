#!/usr/bin/env python3
"""
Development server for web builds.
- Serves static files
- CORS proxy for API calls
- Shutdown endpoint for IDE integration

Usage: python dev_server.py [webroot] [--port PORT]
"""

import http.server
import urllib.request
import urllib.error
import ssl
import os
import sys
import argparse

TARGET_HOST = "https://production-portal.highrisegame.com"

class DevServerHandler(http.server.SimpleHTTPRequestHandler):
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_cors_headers()
        self.end_headers()

    def do_GET(self):
        if self.path.startswith("/api/"):
            self.proxy_request("GET")
        else:
            super().do_GET()

    def do_POST(self):
        if self.path == "/shutdown":
            self.send_response(200)
            self.end_headers()
            print("Shutdown requested, exiting...")
            os._exit(0)
        elif self.path.startswith("/api/"):
            self.proxy_request("POST")
        else:
            self.send_response(404)
            self.end_headers()

    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "*")

    def proxy_request(self, method):
        # Strip /api prefix and proxy to target
        target_url = TARGET_HOST + self.path[4:]  # Remove "/api"
        print(f"[PROXY] {method} {target_url}")

        body = None
        if method == "POST":
            content_length = int(self.headers.get("Content-Length", 0))
            if content_length > 0:
                body = self.rfile.read(content_length)
            print(f"[PROXY] Body size: {len(body) if body else 0}")

        req = urllib.request.Request(target_url, data=body, method=method)

        for header in ["Content-Type", "clientDeviceId", "Authorization"]:
            if header in self.headers:
                req.add_header(header, self.headers[header])
                print(f"[PROXY] Header: {header}: {self.headers[header]}")

        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE

        try:
            with urllib.request.urlopen(req, context=ctx, timeout=30) as response:
                response_body = response.read()
                print(f"[PROXY] Response: {response.status}, {len(response_body)} bytes")
                self.send_response(response.status)
                self.send_cors_headers()
                self.send_header("Content-Type", response.headers.get("Content-Type", "application/octet-stream"))
                self.send_header("Content-Length", len(response_body))
                self.end_headers()
                self.wfile.write(response_body)
        except urllib.error.HTTPError as e:
            print(f"[PROXY] HTTP Error: {e.code}")
            error_body = e.read()
            self.send_response(e.code)
            self.send_cors_headers()
            self.send_header("Content-Length", len(error_body))
            self.end_headers()
            self.wfile.write(error_body)
        except Exception as e:
            import traceback
            print(f"[PROXY] Exception: {e}")
            traceback.print_exc()
            self.send_response(502)
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(str(e).encode())

    def log_message(self, format, *args):
        if "/api/" in (args[0] if args else ""):
            print(f"[API] {args[0]}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Development server for web builds")
    parser.add_argument("webroot", nargs="?", default=".", help="Web root directory")
    parser.add_argument("--port", type=int, default=8080, help="Port to serve on")
    args = parser.parse_args()

    os.chdir(args.webroot)
    print(f"Serving {os.getcwd()} at http://localhost:{args.port}")
    print(f"API proxy: /api/* -> {TARGET_HOST}")
    print("Close browser tab to stop server")

    server = http.server.HTTPServer(("", args.port), DevServerHandler)
    server.serve_forever()
