from http.server import HTTPServer, BaseHTTPRequestHandler
from user_agents import parse

class requestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        
        self.send_response(200)
        self.send_header('content-type', 'text/html')
        self.end_headers()

        user_agent = parse(self.headers['user-agent'])
        
        if self.path.endswith('/'):
            f = open('webpage.html', 'r')
            output = f.read(875)
           
            output +=   '<table class="table table-bordered">' \
                            '<thead>' \
                                '<tr>' \
                                    '<th style="width: 10px">#</th>' \
                                    '<th>Address</th>' \
                                    '<th>Client Port</th>' \
                                '</tr>' \
                            '</thead>' \
                            '<tbody>' \
                                '<tr>' \
                                    '<th>IPv4</th>' \
                                    '<td>' + str(self.client_address[0]) + '</td>' \
                                    '<td>' + str(self.client_address[1]) + '</td>' \
                                '</tr>' \
                            '</tbody>' \
                        '</table>'
            
            output +=   '<table class="table table-bordered">' \
                            '<thead>' \
                                '<tr>' \
                                    '<th style="width: 10px">#</th>' \
                                    '<th>Name</th>' \
                                    '<th>Version</th>' \
                                '</tr>' \
                            '</thead>' \
                            '<tbody>' \
                                '<tr>' \
                                    '<th>Device</th>' \
                                    '<td>' + str(user_agent.device.brand) + '</td>' \
                                    '<td>' + str(user_agent.device.model) + '</td>' \
                                '</tr>' \
                                '<tr>' \
                                    '<th>OS</th>' \
                                    '<td>' + str(user_agent.os.family) + '</td>' \
                                    '<td>' + str(user_agent.os.version_string) + '</td>' \
                                '</tr>' \
                                '<tr>' \
                                    '<th>Browser</th>' \
                                    '<td>' + user_agent.browser.family + '</td>' \
                                    '<td>' + user_agent.browser.version_string + '</td>' \
                                '</tr>' \
                            '</tbody>' \
                        '</table>' \

            output += f.read()
            self.wfile.write(output.encode())

def main():
    PORT = 80
    server_address = ('192.168.1.82', PORT)
    server = HTTPServer(server_address, requestHandler)
    print('Server running on port %s' % PORT)
    server.serve_forever()


if __name__ == '__main__':
    main()
