import socket
import threading
import selectors
import uuid
import logging as log
from Crypto.Cipher import AES


# varibales for managing connections
active_connections = 0


class Server:
    SERVER_VERSION = 3
    MAX_NAME_LEN = 65536
    RECONNECTION_TIME = 3
    PACKET_SIZE = 1024
    def __init__(self):
        self.sel = selectors.DefaultSelector

    def accept(self, sock, mask):
        """ accepting new connection to the server and registering the connection
        to the selector """

        conn, addr = sock.accept() # Should be ready
        print('accepted', conn, 'from', addr)
        conn.setblocking(False)
        self.sel.register(conn, selectors.EVENT_READ, self.read)

    def read(self, conn, mask):
        data = conn.recv(1024) # Should be ready
        if data:
            print('echoing', repr(data), 'to', conn)
            conn.send(data) # Hope it won't block
        else:
            print('closing', conn)
            self.sel.unregister(conn)
            conn.close()

    def write(self, conn, data):
        """ Sends a response to client """


    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as self.s:
            self.s.bind(('localhost', 1234))
            self.s.listen(15)
            self.s.setblocking(False)
            # register selector to events
            self.sel.register(self.s, selectors.EVENT_READ, self.accept)

            while True:
                events = self.sel.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)