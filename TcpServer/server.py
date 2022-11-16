
import socket
import threading
import crc
import selectors

import keys_handler
import protocol
import database
import logging as log


# varibales for managing connections
active_connections = 0

class Server:
    DATABASE = 'server.db'
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
        self.conn = ""
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
        self.conn = conn
        self.sel.register(conn, selectors.EVENT_READ, self.read)

    def read(self, conn, mask):
        print("a client has connected")
        data = conn.recv(1024) # Should be ready
        if data:
            requestHeader = protocol.RequestHeader()
            success = False
            if not requestHeader.unpack(data):
                log.error("Failed to parse request header!")
            else:
                print(requestHeader)
                if requestHeader.code in self.requestHandle.keys():
                    success = self.requestHandle[requestHeader.code](conn, data)  # invoke corresponding handle.
            if not success:  # return generic error upon failure.
                responseHeader = protocol.ResponseHeader(protocol.ResponseCode.REGISTRATION_FAILED.value)
                self.write(conn, responseHeader.pack())
        else:
            self.sel.unregister(conn)
            conn.close()
            print('closing', conn)


    def write(self, conn, data):    #TODO: implement
        """ Sends a response to client """
        size = len(data)
        bytesSent = 0
        while bytesSent < size:
            bytesLeft = size - bytesSent
            if bytesSent > Server.PACKET_SIZE:
                bytesLeft = Server.PACKET_SIZE
            send = data[bytesSent: bytesSent + bytesLeft]
            if len(send) < Server.PACKET_SIZE:
                send += bytearray(Server.PACKET_SIZE - len(send))
            try:
                conn.send(send)
                bytesSent += len(send)
            except Exception as e:
                log.error("Failed to send response to " + conn + "Err: " + str(e))  # todo: check how to improve
                return False
        log.info("Response sent successfully.")
        return True

    def run(self):
        self.database.initialize()
        try:
            self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.s.bind((self.host, self.port))
            self.s.listen(15)
            self.s.setblocking(False)
            # register selector to events
            if self.sel.register(self.s, selectors.EVENT_READ, self.accept):
                print("registered successfully")
        except Exception as e:
            self.lastErr = e
            print(e)
        print(f"Server is listening for connections on port {self.port}")
        while True:
            try:
                events = self.sel.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)
            except Exception as e:
                log.exception(f"Server main loop exception: {e}")



# regustration request from client - saves information to db
    def handleRegistrationRequest(self, conn, data):
        request = protocol.RegistrationRequest()
        response = protocol.RegistrationFailedResponse()    # assuming failure, upon success will be changed
        if not request.unpack(data):
            log.error("Registration Request: Failed parsing request.")
            return False
        try:
            if not request.name.isascii():
                log.error(f"Registration Request: Invalid requested username \"{request.name}\"")
                self.write(conn, response.pack())
                return False
            elif self.database.clientUsernameExists(request.name):
                log.error(f"Registration Request: Username \"{request.name}\" already exists.")
                self.write(conn, response.pack())   # send fail opcode
                return False
        except:
            log.error("Registration Request: Failed to connect to database.")
            self.write(conn, response.pack())   # send fail opcode
            return False

        clnt = self.database.addClient(request.name)  # todo: continue handle adding client to db
        if not clnt:
            log.error(f"Registration Request: Failed to store client {request.name}.")
            self.write(conn, response.pack())   # send fail opcode
            return False
        log.info(f"Successfully registered client {request.name}.")
        response = protocol.RegistrationSucceededResponse()    # change to successful registration
        response.clientID = clnt.id
        print(response)
        return self.write(conn, response.pack())

    def handlePublicKeyRequest(self, conn, data):
        """Received public key from client, generate AES key and encryp it with the public key """
        request = protocol.PublicKeyRequest()
        response = protocol.RegistrationFailedResponse() # assume failure until validated
        if not request.unpack(data):
            log.error("PublicKey Request: Failed to parse request!")
            return False
        if not self.database.clientExists(request.header.clientID):
            log.error("PublicKey Request: Client does not exists!")
            self.write(conn, response.pack()) # sending registartion failed to client
            return False
        key = self.database.setClientsPublicKey(request.header.clientID, request.publicKey)
        if not key:
            log.error(f"PublicKey Request: Failed to set public key.")
            self.write(conn, response.pack()) # sending registartion failed to client
            return False
        rsa_key = request.publicKey
        # todo: handle generating symmetric key
        symmetricKey = self.database.setClientsSymmetricKey(request.header.clientID)
        if not symmetricKey:
            log.error(f"Failed to generate symmetric key for client {request.header.clientID}")
            return False
        response = protocol.PublicKeyResponse()     # Client exists
        response.clientID = request.header.clientID
        encrypted_aes = keys_handler.encryptAesKeyWithRsaPublic(symmetricKey, rsa_key)
        response.symmetricKey = encrypted_aes
        response.header.payloadSize = protocol.CLIENT_ID_SIZE + protocol.SYMMETRIC_KEY_SIZE
        print(response)
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
        self.database.storeFile(request.header.clientID, request.fileName, request.fileContent)
        fileData = request.fileContent
        calc = crc.crc32()
        calc.update(fileData)
        response.checksum = calc.digest()   # fill crc
        print(response)
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


