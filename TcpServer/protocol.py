import struct
from enum import Enum
import logging as log

# All sizes are in BYTES.
SERVER_VERSION = 3
MAX_INCOMING_PAYLOAD_SIZE = (16 + 4 + 255 + 4)    # payload of FileReceivedValidCrcResponseMessage
CLIENT_ID_SIZE = 16
CLIENT_NAME_SIZE = 255
PUBLIC_KEY_SIZE = 160       # defined in protocol. 1024 bits
SYMMETRIC_KEY_SIZE = 16     # defined in protocol. 128 bits
FILE_NAME_SIZE = 255
HEADER_SIZE = 7             # header size without ClientID
PACKET_SIZE = 1024


class RequestCode(Enum):
    REQUEST_REGISTRATION = 1100
    REQUEST_PUBLIC_KEY = 1101
    REQUEST_TO_SEND_FILE = 1102
    REQUEST_CRC_VALID = 1104          # not really a request as much as status report to server
    INVALID_CRC_RESEND_REQUEST = 1105
    INVALID_CRC_ABORT = 1106


class ResponseCode(Enum):
    REGISTRATION_SUCCEEDED = 2100
    REGISTRATION_FAILED = 2101
    PUBLIC_KEY_RECEIVED = 2102      # send out AES key
    MSG_RECEIVED_CRC = 2103
    MSG_RECEIVED = 2104


class MessageType(Enum):
    MSG_SYMMETRIC_KEY_REQUEST = 1
    MSG_SYMMETRIC_KEY_SEND = 2
    MSG_TEXT = 3
    MSG_FILE = 4

# Request Header as received from Client
class RequestHeader:
    def __init__(self):
        self.clientID = b""
        self.version = 0      # 1 byte
        self.code = 0         # 2 bytes
        self.payloadSize = 0  # 4 bytes
        self.SIZE = CLIENT_ID_SIZE + HEADER_SIZE

    def unpack(self, data):
        """ Little Endian unpack Request Header """
        try:
            self.clientID = struct.unpack(f"<{CLIENT_ID_SIZE}s", data[:CLIENT_ID_SIZE])[0]
            headerData = data[CLIENT_ID_SIZE:CLIENT_ID_SIZE + HEADER_SIZE]
            self.version, self.code, self.payloadSize = struct.unpack("<BHI", headerData)
            return True
        except:
            self.__init__()  # flush values
            return False

    def __str__(self):
        return f"Client's id: {self.clientID}, version: {self.version}, code: {self.code}, payload size: {self.payloadSize} "

class ResponseHeader:
    def __init__(self, code):
        self.version = SERVER_VERSION  # 1 byte
        self.code = code               # 2 bytes
        self.payloadSize = 0           # 4 bytes
        self.SIZE = HEADER_SIZE

    def pack(self):
        """ Little Endian pack Response Header """
        try:
            return struct.pack("<BHI", self.version, self.code, self.payloadSize)
        except:
            return b""

    def __str__(self):
        return f"version: {self.version}, code: {self.code}, payload size: {self.payloadSize} "




#----------------- REQUESTS FROM CLIENT --------------
class RegistrationRequest:
    def __init__(self):
        self.header = RequestHeader()
        self.name = b""

    def unpack(self, data):
        """ Little Endian unpack Request Header and Registration data """
        if not self.header.unpack(data):
            return False
        try:
            # trim the byte array after the nul terminating character.
            nameData = data[self.header.SIZE:self.header.SIZE + CLIENT_NAME_SIZE]
            self.name = str(struct.unpack(f"<{CLIENT_NAME_SIZE}s", nameData)[0].partition(b'\0')[0].decode('utf-8'))
            return True
        except:
            self.name = b""
            return False

    def __str__(self):
        return "RegistrationRequest:\n" + str(self.header) + f"\nClient's name: {self.name}"



class PublicKeyRequest:
    """Client sends a public key and waits for a return of an AES key """
    def __init__(self):
        self.header = RequestHeader()
        self.clientName = b""
        self.publicKey = b""

    def unpack(self, data):
        """ Little Endian unpack Request Header and client ID """
        if not self.header.unpack(data):
            return False
        try:
            nameData = data[self.header.SIZE:self.header.SIZE + CLIENT_NAME_SIZE]
            self.clientName = str(struct.unpack(f"<{CLIENT_NAME_SIZE}s", nameData)[0].partition(b'\0')[0].decode('utf-8'))
            offset = self.header.SIZE + CLIENT_NAME_SIZE
            publicKeyData = data[offset:offset + PUBLIC_KEY_SIZE]
            self.publicKey= struct.unpack(f"<{PUBLIC_KEY_SIZE}s", publicKeyData)[0]
            return True
        except:
            self.clientName = b""
            self.publicKey = b""
        return False

    def __str__(self):
        return "PublicKeyRequest:\n" + str(self.header) + f"\nClient's name: {self.clientName}\nPublicKey: {str.encode(str(self.publicKey),'utf-8').hex()} "


class RequestToSendFile:
    def __init__(self):
        self.header = RequestHeader()
        self.contentSize = 0
        self.fileName = b""
        self.fileContent = b""      # received as binary

    def unpack(self, conn, data):
        """ Little Endian unpack Request Header and message data """
        packetSize = len(data)
        if not self.header.unpack(data):
            return False
        try:
            self.contentSize = struct.unpack("<I", data[self.header.SIZE: self.header.SIZE + 4])[0]
            offset = self.header.SIZE + 4
            # trim the byte array after the null terminating character.
            fileNameData = data[offset:offset + FILE_NAME_SIZE]
            self.fileName = str(struct.unpack(f"<{FILE_NAME_SIZE}s", fileNameData)[0].partition(b'\0')[0].decode('utf-8'))
            offset += FILE_NAME_SIZE
            encryptedContentSize =  self.header.payloadSize - FILE_NAME_SIZE - 4 # 4 bytes of int; the encryptedContent might pe different in size than the actual content
            bytesRead = encryptedContentSize
            if bytesRead > PACKET_SIZE:
                bytesRead = PACKET_SIZE - offset
            self.fileContent = struct.unpack(f"<{bytesRead}s", data[offset: offset + bytesRead])[0]
            while bytesRead < encryptedContentSize:
                data = conn.recv(packetSize)  # reuse first size of data.
                dataSize = len(data)
                if (encryptedContentSize - bytesRead) < dataSize:
                    dataSize = encryptedContentSize - bytesRead
                self.fileContent += struct.unpack(f"<{dataSize}s", data[:dataSize])[0]
                bytesRead += dataSize
            return True
        except Exception as e:
            log.error(f"{e}")
            self.contentSize = 0
            self.fileName = b""
            self.fileContent = b""
            return False

    def __str__(self):
        return "RequestToSendFile:\n" + str(self.header) + f"\nContent size: {self.contentSize}" \
                                                           f"\nfile name: {self.fileName} " \
                                                           f"\nfile Content:{self.fileContent}"


class CrcStatusRequest:
    def __init__(self):
        self.header = RequestHeader()
        self.clientID = b""
        self.fileName = b""

    def unpack(self, data):
        """ Little Endian unpack Request Header and client ID """
        if not self.header.unpack(data):
            return False
        try:
            clientID = data[self.header.SIZE:self.header.SIZE + CLIENT_ID_SIZE]
            self.clientID = struct.unpack(f"<{CLIENT_ID_SIZE}s", clientID)[0]
            offset = self.header.SIZE + CLIENT_ID_SIZE
            fileNameData = data[offset:offset + FILE_NAME_SIZE]
            self.fileName = str(struct.unpack(f"<{FILE_NAME_SIZE}s", fileNameData)[0].partition(b'\0')[0].decode('utf-8'))
            return True
        except:
            self.clientID = b""
            self.fileName = b""
            return False

    def __str__(self):
        return "CrcStatusRequest:\n" + str(self.header) + f"\nClient's ID: {self.clientID}" \
                                                          f"\nFile name: {self.fileName}"




# todo: maybe redundant
class CrcValidRequest(CrcStatusRequest):
    def __init__(self):
        super().__init__()

class CrcNotValidRequest(CrcStatusRequest):
    def __init__(self):
        super().__init__()


class CrcNotValidAbortingRequest(CrcStatusRequest):
    def __init__(self):
        super().__init__()


#----------------- RESPONSES TO CLIENT --------------
class RegistrationSucceededResponse:
    def __init__(self):
        self.header = ResponseHeader(ResponseCode.REGISTRATION_SUCCEEDED.value)
        self.clientID = b""
        self.header.payloadSize = CLIENT_ID_SIZE
    def pack(self):
        """ Little Endian pack Response Header and client ID """
        try:
            data = self.header.pack()
            data += struct.pack(f"<{CLIENT_ID_SIZE}s", self.clientID)
            return data
        except:
            return b""

    def __str__(self):
        return "RegistrationSucceededResponse:\n" + str(self.header) + f" Client ID: {self.clientID.hex(' ',1)}"




class RegistrationFailedResponse:
    def __init__(self):
        self.header = ResponseHeader(ResponseCode.REGISTRATION_FAILED.value)

    def pack(self):
        """ Little Endian pack Response Header with no payload """
        try:
            data = self.header.pack()
            return data
        except:
            return b""
    def __str__(self):
        return "RegistrationFailedResponse:\n" + str(self.header)


class PublicKeyResponse:
    def __init__(self):
        self.header = ResponseHeader(ResponseCode.PUBLIC_KEY_RECEIVED.value)
        self.clientID = b""
        self.symmetricKey = b""
        self.header.payloadSize = CLIENT_ID_SIZE + SYMMETRIC_KEY_SIZE

    def pack(self):
        """ Little Endian pack Response Header and Public Key """
        try:
            data = self.header.pack()
            data += struct.pack(f"<{CLIENT_ID_SIZE}s", self.clientID)
            data += struct.pack(f"<{len(self.symmetricKey)}s", self.symmetricKey)
            return data
        except:
            return b""


    def __str__(self):
        return "PublicKeyResponse:\n" + str(self.header) + f"\nClient's ID: {self.clientID}" \
                                                           f"\nSymmetricKey: {self.symmetricKey} "




class FileReceivedCrcValueResponse:
    def __init__(self):
        self.header = ResponseHeader(ResponseCode.MSG_RECEIVED_CRC.value)
        self.clientID = b""
        self.contentSize = 0
        self.fileName = b""
        self.checksum = 0
        self.header.payloadSize = CLIENT_ID_SIZE + FILE_NAME_SIZE + 8 # 4+4 for content size and checksum size

    def pack(self):
        """ Little Endian pack Response Header and Public Key """
        try:
            data = self.header.pack()
            data += struct.pack(f"<{CLIENT_ID_SIZE}s", self.clientID)
            data += struct.pack("<I", self.contentSize)
            filename = self.fileName
            if type(self.fileName) is str:
                filename = str.encode(filename,'utf-8')
            data += struct.pack(f"<{FILE_NAME_SIZE}s", filename)
            data += struct.pack("<I", self.checksum)
            return data
        except Exception as e:
            log.error(f"{e}")
            return b""


    def __str__(self):
        return "FileReceivedCrcValueResponse:\n" + str(self.header) + f"\nClient's ID: {self.clientID}" \
                                                           f"\nContent size: {self.contentSize}" \
                                                                      f"\nFile name: {self.fileName}" \
                                                                      f"\nCheckSum: {self.checksum} "




class ReceivingApprovedResponse:
    def __init__(self):
        self.header = ResponseHeader(ResponseCode.MSG_RECEIVED.value)

    def pack(self):
        """ Little Endian pack Response Header and Public Key """
        try:
            data = self.header.pack()
            return data
        except:
            return b""
