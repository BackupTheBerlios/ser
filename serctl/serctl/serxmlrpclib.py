
import xmlrpclib

class Transport(xmlrpclib.Transport):

    def request(self, host, handler, request_body, verbose=0):
        # issue XML-RPC request

        h = self.make_connection(host)
        if verbose:
            h.set_debuglevel(1)

        self.send_request(h, handler, request_body)
        self.send_host(h, host)
        self.send_user_agent(h)
        self.send_content(h, request_body)

        errcode, errmsg, headers = h.getreply()

        if errcode != 200:
            raise ProtocolError(
                host + handler,
                errcode, errmsg,
                headers
                )

        self.verbose = verbose

        try:
            sock = h._conn.sock
        except AttributeError:
            sock = None

        leng = int(headers.getheader("Content-Length", 1024))
        return self._parse_response(h.getfile(), sock, leng)


    def parse_response(self, file, body_len = 1024):
        # compatibility interface
        return self._parse_response(file, None, body_len)

    def _parse_response(self, file, sock, body_len = 1024):
        # read response from input file/socket, and parse it

        p, u = self.getparser()

        while 1:
            if sock:
                response = sock.recv(body_len)
                body_len -= len(response)
            else:
                response = file.read(body_len)
                body_len -= len(response)
            if not response:
                break
            if self.verbose:
                print "body:", repr(response)
            p.feed(response)

        file.close()
        p.close()

        return u.close()


xmlrpclib.Transport = Transport
from xmlrpclib import *
