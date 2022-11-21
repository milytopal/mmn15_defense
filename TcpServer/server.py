import socket
import threading
from crccheck.crc import Crc32
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
        #self.sel = selectors.DefaultSelector()
        self.database = database.Database(Server.DATABASE)
        self.lock = threading.Lock()
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
        #self.sel.register(conn, selectors.EVENT_READ, self.read)

    def read(self, conn):
        """Read data from client and parse it"""
        while True:
            data = conn.recv(self.PACKET_SIZE)
            if data:
                request_header = protocol.RequestHeader()
                success = False
                if not request_header.unpack(data):
                    log.error("Failed parsing request data header")
                else:
                    # checking if code exists
                    if request_header.code in self.requestHandle.keys():
                        success = self.requestHandle[request_header.code](conn, data)
                # if we got any issues handling the request
                if not success:
                    response_header = protocol.ResponseHeader(
                        protocol.ResponseCode.REGISTRATION_FAILED.value
                    )
                    self.write(conn, response_header.pack())
                    self.lock.release()
                    break
            else:
                self.lock.release()
                log.info("Communication")
                break
        conn.close()

    def write(self, conn, data):
        """Send response to client
        Args:
            conn (Socket): Socket to client
            data (str): data to send the client
        """
        size = len(data)
        sent = 0
        while sent < size:
            left_over = size - sent
            if left_over > self.PACKET_SIZE:
                left_over = self.PACKET_SIZE
            data_to_send = data[sent : sent + left_over]
            if len(data_to_send):
                data_to_send += bytearray(self.PACKET_SIZE - len(data_to_send))
            try:
                conn.send(data_to_send)
                sent += len(data_to_send)
            except:
                log.error(f"ERROR: failed sending a response to {conn}")
        log.info(f"Response has been sent succesfully")
        return True

    def run(self):
        """Start Listen for connections"""
        try:
            sock = socket.socket()
            sock.bind((self.host, self.port))
            sock.listen(15)
            log.info(f"Server is listening for connection on port {self.port}")
            while True:
                conn, address = sock.accept()
                self.lock.acquire()
                log.info('Connected to: ' + address[0] + ':' + str(address[1]))
                t1 = threading.Thread(target=self.read, args=(conn,),name='clientThread')
                t1.start()
        except Exception as e:
            log.error(f"Failed openning socket : {e}")
            return False


    def handleRegistrationRequest(self, conn, data):
        """ regustration request from client - saves information to db """
        request = protocol.RegistrationRequest()
        response = protocol.RegistrationFailedResponse()    # assuming failure, upon success will be changed
        if not request.unpack(data):
            log.error("Registration Request: Failed parsing request.")
            return False
        try:
            if not request.name.isascii():
                self.write(conn, response.pack())  # sending registartion failed to client
                log.error(f"Registration Request: Invalid requested username \"{request.name}\"")
                return False
            elif self.database.clientUsernameExists(request.name):
                self.write(conn, response.pack())  # sending registartion failed to client
                log.error(f"Registration Request: Username \"{request.name}\" already exists.")
                return False
        except:
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("Registration Request: Failed to connect to database.")
            return False

        clnt = self.database.addClient(request.name)  # todo: continue handle adding client to db
        if not clnt:
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error(f"Registration Request: Failed to store client {request.name}.")
            return False
        log.info(f"Successfully registered client {request.name}.")
        response = protocol.RegistrationSucceededResponse()    # change to successful registration
        response.clientID = clnt.id
        return self.write(conn, response.pack())

    def handlePublicKeyRequest(self, conn, data):
        """Received public key from client, generate AES key and encryp it with the public key """
        request = protocol.PublicKeyRequest()
        response = protocol.RegistrationFailedResponse() # assume failure until validated
        if not request.unpack(data):
            log.error("PublicKey Request: Failed to parse request!")
            return False
        if not self.database.clientExists(request.header.clientID):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("PublicKey Request: Client does not exists!")
            return False
        key = self.database.setClientsPublicKey(request.header.clientID, request.publicKey)
        if not key:
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error(f"PublicKey Request: Failed to set public key.")
            return False
        rsa_key = request.publicKey
        symmetricKey = self.database.setClientsSymmetricKey(request.header.clientID)
        if not symmetricKey:
            log.error(f"Failed to generate symmetric key for client {request.header.clientID}")
            return False
        response = protocol.PublicKeyResponse()     # Client exists
        response.clientID = request.header.clientID
        encrypted_aes = keys_handler.encryptAesKeyWithRsaPublic(symmetricKey, rsa_key)
        response.symmetricKey = encrypted_aes
        response.header.payloadSize = protocol.CLIENT_ID_SIZE + len(encrypted_aes)
        log.info(f"Public Key response was successfully built to clientID ({request.header.clientID}).")
        return self.write(conn, response.pack())


    def handleSendFileRequest(self, conn, data):
        """ recieve encrypted file from client, decryp it and calculate checksum """
        request = protocol.RequestToSendFile()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(conn, data):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("Send File Request: Failed to parse request!")
            return False
        if not self.database.clientExists(request.header.clientID):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("Send File Request: Client does not exists!")
            return False
        response = protocol.FileReceivedCrcValueResponse()
        response.contentSize = request.contentSize
        response.clientID = request.header.clientID
        response.fileName = request.fileName

         # Decrypt the data with the AES session key
        aesKey = self.database.getClientsSymmetricKey(response.clientID)
        decrypted_content = keys_handler.decrypt(request.fileContent,aesKey, request.contentSize)

        file = self.database.storeFileinDB(request.header.clientID, request.fileName, decrypted_content)
        if file:
            print(f"file crc: {file.crc}")
            response.checksum = file.crc

        return self.write(conn, response.pack())


    def handleCrcValidRequest(self, conn, data):
        request = protocol.CrcValidRequest()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(data):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("Crc Valid Request: Failed to parse request!")
            return False
        if not self.database.clientExists(request.header.clientID):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("Crc Valid Request: Client does not exists!")
            return False
        if not self.database.updateVerification(request.clientID, request.fileName, True):
            self.write(conn, response.pack())  # sending registartion failed to client
            log.error("Crc Valid Request: Failed to update db!")
            return False
        response = protocol.ReceivingApprovedResponse()
        # set verification flag to True and save file in file system
        return self.write(conn, response.pack())

    def handleInvalidCrcResendRequest(self, conn, data):
        request = protocol.CrcNotValidRequest()
        response = protocol.RegistrationFailedResponse()
        if not request.unpack(data):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("InvalidCrcResendRequest: Failed to parse request!")
            return False
        if not self.database.clientExists(request.header.clientID):
            self.write(conn, response.pack()) # sending registartion failed to client
            log.error("InvalidCrcResendRequest: Client does not exists!")
            return False
        log.info("InvalidCrcResendRequest: Client responded Crc invalid - Retrying!")

        response = protocol.FileReceivedCrcValueResponse()
        fileInstance = self.database.getFile(request.clientID, request.fileName)
        if not fileInstance:
            self.write(conn, response.pack())  # sending registartion failed to client
            log.error("InvalidCrcResendRequest: Failed to Retrieve File From DB!")
            return False
        response.contentSize = len(fileInstance.content)        # content stored decrypted already
        response.clientID = request.header.clientID
        response.fileName = request.fileName
        crc = Crc32.calc(fileInstance.content)                  # calculating again
        response.checksum = crc
        return self.write(conn, response.pack())



    def handleInvalidCrcAbortInteractionRequest(self, conn, data):
        request = protocol.CrcNotValidAbortingRequest()
        if not request.unpack(data):
            log.error("InvalidCrcResendRequest: Failed to parse request!")
            return False
        clientname = self.database.getClientNameById(request.clientID)
        if not clientname:
            log.error(f"failed to find client name for id {str(request.clientID)}")
        try:
            clientname = bytes.decode(clientname, 'utf-8')
        except:
            pass
        filename = request.fileName
        try:
            filename = bytes.decode(filename, 'utf-8')
        except:
            pass
        log.error(f"""Calculated Crc Was Not Valid for ClientName: {clientname} file: {filename}
        Ending Session with Client """)
        self.sel.unregister(conn)
        conn.close()
        print('closing', str(conn))
        return False



