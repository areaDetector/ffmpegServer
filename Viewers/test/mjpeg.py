#!/usr/bin/env dls-python2.6

"naff mjpeg server to test ffmpeg"

import BaseHTTPServer, SimpleHTTPServer, sys, time
from SocketServer import ThreadingMixIn

# Harriet
cats = [file(sys.path[0] + "/cat1.jpg").read(),
        file(sys.path[0] + "/cat2.jpg").read()]

counter = 0

class Request(BaseHTTPServer.BaseHTTPRequestHandler):
    
    def do_GET(self):
        
        global counter
        
        self.send_response(200)
        # ffmpeg doesn't use the mime multipart boundaries
        # or the content length
        # it looks for ffd8 (start of image) and ffd9 (end of image)
        # markers in the byte stream
        # as jpeg is huffman encoded these can be guaranteed not to
        # occur inside the image
        self.send_header("Content-type", "text/plain")
        self.end_headers()

        for n in range(100):
            print "frame out %d" % counter
            self.wfile.write(cats[counter % 2])
            counter += 1
            time.sleep(1)
        
class ThreadedHTTPServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass

def run(server_class=BaseHTTPServer.HTTPServer,
        handler_class=BaseHTTPServer.BaseHTTPRequestHandler, port = 8000):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()

if __name__ == "__main__":
    port = int(sys.argv[1])
    run(server_class = ThreadedHTTPServer, handler_class = Request, port = port)
    
