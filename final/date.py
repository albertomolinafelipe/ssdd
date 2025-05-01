from http.server import BaseHTTPRequestHandler, HTTPServer
from datetime import datetime

class SimpleHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/datetime':
            now = datetime.now()
            date_time_str = now.strftime("%d/%m/%Y %H:%M:%S")
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(date_time_str.encode())
        else:
            self.send_response(404)
            self.end_headers()

if __name__ == '__main__':
    server = HTTPServer(('127.0.0.1', 8000), SimpleHandler)
    server.serve_forever()
