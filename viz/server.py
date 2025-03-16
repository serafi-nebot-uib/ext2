#!/usr/bin/env python3

import json
import random
from http.server import BaseHTTPRequestHandler, HTTPServer

class HTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/data":
            def generate_tree(depth, max_depth, max_children, current_id):
                node = {"id": str(current_id), "value": random.randint(1, 100), "children": []}
                current_id += 1
                if depth < max_depth:
                    num_children = random.randint(0, max_children)
                    for _ in range(num_children):
                        child, current_id = generate_tree(depth + 1, max_depth, max_children, current_id)
                        node["children"].append(child)
                return node, current_id
            max_depth = 3
            max_children = 3
            root, _ = generate_tree(0, max_depth, max_children, 0)
            # Send JSON response
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(root).encode("utf-8"))
        else:
            try:
                if self.path == "/":
                    self.path = "/index.html"
                with open("./static" + self.path, "rb") as f:
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
