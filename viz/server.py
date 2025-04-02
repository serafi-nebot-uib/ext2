#!/usr/bin/env python3

import json
from pathlib import Path
from structs import inode_ptr_tree_from_file
from http.server import BaseHTTPRequestHandler, HTTPServer

class HTTPHandler(BaseHTTPRequestHandler):
  def do_GET(self):
    if self.path == "/data":
      ninode = 0
      root = { "name": f"inode {ninode}", "children": inode_ptr_tree_from_file("../disco", ninode) }
      self.send_response(200)
      self.send_header("Content-type", "application/json")
      self.end_headers()
      self.wfile.write(json.dumps(root).encode("utf-8"))
    else:
      try:
        if self.path == "/":
          self.path = "/index.html"
        with Path("./static" + self.path).open("rb") as f:
          self.send_response(200)
          if self.path.endswith(".html"):
            self.send_header("Content-type", "text/html")
          elif self.path.endswith(".js"):
            self.send_header("Content-type", "application/javascript")
          elif self.path.endswith(".css"):
            self.send_header("Content-type", "text/css")
          self.end_headers()
          self.wfile.write(f.read())
      except FileNotFoundError:
        self.send_response(404)
        self.end_headers()

HOST, PORT = "127.0.0.1", 8000
server = HTTPServer((HOST, PORT), HTTPHandler)
print(f"server running on http://{HOST}:{PORT}")
server.serve_forever()
