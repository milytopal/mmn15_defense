
import socket
import threading
import crc
import selectors
import protocol
import database
import logging as log


# varibales for managing connections
active_connections = 0

class Server:
    SERVER_VERSION = 3
    MAX_NAME_LEN = 65536
    RECONNECTION_TIME = 3
    PACKET_SIZE = 1024
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sel = selectors.DefaultSelector()
        self.database = database.Database(Server.DATABASE)
        self.lastErr = ""  # Last Error description.
        # all functions get connection and data from read function
        self.requestHandle = {
            protocol.RequestCode.REQUEST_REGISTRATION.value: self.handleRegistrationRequest,
            protocol.RequestCode.REQUEST_PUBLIC_KEY.value: self.handlePublicKeyRequest,
            protocol.RequestCode.REQUEST_TO_SEND_FILE.value: self.handleSendFileRequest,
            protocol.RequestCode.REQUEST_CRC_VALID.value: self.handleCrcValidRequest,
            protocol.RequestCode.INVALID_CRC_RESEND_REQUEST.value: self.handleInvalidCrcResendRequest,
            protocol.RequestCode.INVALID_CRC_ABORT.value: self.handleInvalidCrcAbortInteractionRequest
        }

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

    def write(self, conn, data): #TODO: implement
        """ Sends a response to client """
        size = len(data)
        bytesSent = 0
        while bytesSent > size:
            bytesLeft = size = bytesSent
            if bytesSent > Server.PACKET_SIZE:
                bytesLeft = Server.PACKET_SIZE
            send = data[bytesSent: bytesSent + bytesLeft]
            if len(send) < Server.PACKET_SIZE:
                send += bytearray(Server.PACKET_SIZE - len(send))
            try:
                conn.send(send)
                bytesSent += len(send)
            except Exception as e:
                log.error("Failed to send response to " + conn + "Err: " + str(e)) # todo: check how to improve
                return False
        log.info("Response sent successfully.")
        return True

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

# regustration request from client - saves information to db
    def handleRegistrationRequest(self, conn, data):
        request = protocol.RegistrationRequest()
        response = protocol.RegistrationFailedResponse()    # assuming failure, upon success will be changed
        if not request.unpack(data):
            log.error("Registration Request: Failed parsing request.")
            return False
        try:
            if not request.name.isalnum():
                log.error(f"Registration Request: Invalid requested username ({request.name}))")
            elif self.database.clientUsernameExists(request.name):
                log.error(f"Registration Request: Username ({request.name}) already exists.")
            self.write(conn, response.pack())
            return False
        except:
            log.error("Registration Request: Failed to connect to database.")
            self.write(conn, response.pack())
            return False

        clnt = database.addClient(request.name) # todo: continue handle adding client to db
        if not self.database.storeClient(clnt):
            log.error(f"Registration Request: Failed to store client {request.name}.")
            self.write(conn, response.pack())
            return False
        log.info(f"Successfully registered client {request.name}.")
        response = protocol.RegistrationFailedResponse()    # change to successful registration
        response.clientID = clnt.ID
        return self.write(conn, response.pack())

    def handlePublicKeyRequest(self, conn, data):
        """Received public key from client, generate AES key and encryp it with the public key """
        request = protocol.PublicKeyRequest()
        response = protocol.RegistrationFailedResponse() # assume failure until validated
        if not request.unpack(data):
            log.error("PublicKey Request: Failed to parse request!")
        if not self.database.clientExists(request.header.clientID):
            log.error("PublicKey Request: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client
        key = self.database.setClientsPublicKey(request.header.clientID, request.publicKey)
        if not key:
            log.error(f"PublicKey Request: Failed to set public key.")
            self.write(conn, response.pack()) # sending registartion failed to client
            return False
        # todo: handle generating symmetric key
        response = protocol.PublicKeyResponse()     # Client exists
        response.clientID = request.header.clientID
        response.symmetricKey = self.database.getClientsSymmetricKey(response.clientID)
        response.header.payloadSize = protocol.CLIENT_ID_SIZE + protocol.PUBLIC_KEY_SIZE
        log.info(f"Public Key response was successfully built to clientID ({request.header.clientID}).")
        return self.write(conn, response.pack())

    def handleSendFileRequest(self, conn, data):
        request = protocol.RequestToSendFile()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(conn, data):
            log.error("Send File Request: Failed to parse request!")
        if not self.database.clientExists(request.header.clientID):
            log.error("Send File Request: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client
        response = protocol.FileReceivedCrcValueResponse()
        response.contentSize = len(request.fileContent)
        response.clientID = request.header.clientID
        response.fileName = request.fileName
        # todo: add decryption step - need to calculate after decryption
        fileData = bytes(request.fileContent)
        calc = crc.crc32()
        calc.update(fileData)
        response.checksum = calc.digest()   # fill crc

        self.write(conn, response.pack())




    def handleCrcValidRequest(self, conn, data):
        request = protocol.CrcValidRequest()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(data):
            log.error("Crc Valid Request: Failed to parse request!")
        if not self.database.clientExists(request.header.clientID):
            log.error("Crc Valid Request: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client
        response = protocol.ReceivingApprovedResponse()
        # todo: Handle (Somehow) saving the file parameters in the database
        self.write(conn, response.pack())

    def handleInvalidCrcResendRequest(self, conn, data):
        request = protocol.CrcNotValidRequest()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(data):
            log.error("InvalidCrcResendRequest: Failed to parse request!")
        if not self.database.clientExists(request.header.clientID):
            log.error("InvalidCrcResendRequest: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client
        log.info("InvalidCrcResendRequest: Client responded Crc invalid - Retrying!")



    def handleInvalidCrcAbortInteractionRequest(self, conn, data):
        request = protocol.CrcNotValidAbortingRequest()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(data):
            log.error("InvalidCrcAbortInteractionRequest: Failed to parse request!")
        if not self.database.clientExists(request.header.clientID):
            log.error("InvalidCrcAbortInteractionRequest: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client


