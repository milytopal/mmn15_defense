import pathlib

import protocol
import utils
import server
import struct
import logging as log
import os
from pathlib import Path
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES
from Crypto import Random

key = RSA.generate(1024).public_key()




PORT_INFO = "port.info"
logger = log.getLogger(__name__)
FORMAT = "[%(filename)s:%(lineno)s - %(funcName)20s() ] %(message)s"
log.basicConfig(format=FORMAT)
logger.setLevel(log.DEBUG)

registration_request = protocol.RegistrationRequest()
registration_request.header.clientID = b"18ea0c6f01000000fc8ae70001000080"
registration_request.name = b"milytttttopal"
registration_request.header.code = (protocol.RequestCode.REQUEST_REGISTRATION).value
registration_request.header.payloadSize = protocol.CLIENT_NAME_SIZE
data = struct.pack(f"<{protocol.CLIENT_ID_SIZE}s", registration_request.header.clientID)
data += struct.pack("<BHI", registration_request.header.version, registration_request.header.code,
                    registration_request.header.payloadSize)
data += struct.pack(f"<{protocol.CLIENT_NAME_SIZE}s", registration_request.name)

Registaion_Request = data

send_file_req = protocol.RequestToSendFile()
send_file_req.header.code = (protocol.RequestCode.REQUEST_TO_SEND_FILE).value
send_file_req.header.clientID = b"e750c4869a0fee4fa76e7e2bba229f4f"
send_file_req.fileName = b"DummyText.txt"
send_file_req.fileContent = bytes("jhfvnksdcsdcewwcdcsddsknmcoinjnedkjn",'utf-8')
send_file_req.contentSize = len(send_file_req.fileContent)
send_file_req.header.payloadSize = send_file_req.header.SIZE + protocol.FILE_NAME_SIZE + 8 + send_file_req.contentSize

data = struct.pack(f"<{protocol.CLIENT_ID_SIZE}s", send_file_req.header.clientID)
data += struct.pack("<BHI", send_file_req.header.version, send_file_req.header.code,
                    send_file_req.header.payloadSize)
data += struct.pack(f"<I{protocol.FILE_NAME_SIZE}s",send_file_req.contentSize, send_file_req.fileName)
data += struct.pack(f"<{send_file_req.contentSize}s", send_file_req.fileContent)

SendFileReq = data


public_key_req = protocol.PublicKeyRequest()
public_key_req.header.code = (protocol.RequestCode.REQUEST_PUBLIC_KEY).value
public_key_req.header.clientID = bytearray.fromhex("d4f44e0aa961f94ba35b6085c486dace")
public_key_req.header.clientID = b'\xce$\x05cw\xbe\xbbK\xad\x081\ry\x84\xe8Y'
temp_key = key.export_key('DER')
public_key_req.publicKey = temp_key[0:-1]
print(f"key len: {len(public_key_req.publicKey)}")
# public_key_req.publicKey = str.encode("MIICcwIBADANBgkqhkiG9w0BAQEFAASCAl0wggJZAgEAAoGBALdOMw6QxI/yGmAl+RmkCGHjhwluVs2H60Moqvo8y/wuY9f7lkoauyOpczAbjri0p8wW78jvYK3p9yF400BsEhQpD9EyuEKKMCqDj5gpXd+wdGhKuMK+nORDZ0JvgOaOwf/gZfRxiPQzOyxqlXBrksjtKX3ohIeW66FxskC6FiaTAgERAoGAGSjZ1NKdfStE4AU2RMYzWLrWTJasiqQ5Y5IhgbgH62W9Y+tQ3P6mPBxHBppe4iLVxq3QmRHMA8rHkSSpi1ogmR9zsWBCyLvRgs4mKfVKQ0ybnLy8/x5+4tJuV2gPs29UyuABiy8hR68lZ4w5rzPd3Kf5R+k9teqYO/ff/m76InkCQQDh2sy7qhckBmuqNr3uvXwbRSBiIVV9+HTCiKeZz4DTE2bWUSN8TwVrP5/l3RgWgnkTdy3oaqEGnF21bech5wxtAkEAz8WKcJVQVuoqaR460cx5QnhkQFsF9L7WXyhnKKy4ItccF8BtMCLkgsZ1hxQStptRWwfWXMYCUNWO0uRlAewe/wJAJ9tRTksxQpe4pZEwk4rZqnWcTY1pcJVBx/n/dX74nbghNOEkYTsfEu0cN598uK2c5VFEVjDvPWbjXECSMyjF9QJAJKpjuYPC4ilSx0GR6Mm7C7rkg9PT3+VxH9n0FjyY9xbm1wPXCH6gy8irVBKZ4/1Kl5f4pvXELGHsBxk/AFbYSwJAM+nG7oVwxOpAIlEwG8XAzS8gIz75uW7V5jRJitPAwwTcxE/5A0eeCmBx31fNtvPIpilLecHuJGje55kw9NCs1g==",'utf-8')
public_key_req.header.payloadSize = protocol.PUBLIC_KEY_SIZE + protocol.CLIENT_NAME_SIZE
public_key_req.clientName = b"milytttttopal"


data = struct.pack(f"<{protocol.CLIENT_ID_SIZE}s",public_key_req.header.clientID)
data += struct.pack("<BHI", public_key_req.header.version, public_key_req.header.code,
                    public_key_req.header.payloadSize)
data += struct.pack(f"<{protocol.CLIENT_NAME_SIZE}s",public_key_req.clientName)
data += struct.pack(f"<{protocol.PUBLIC_KEY_SIZE}s", (public_key_req.publicKey))
publicKeyReq = data

if __name__ == '__main__':
    port = utils.init_port_config(PORT_INFO)
    if port is None:
        utils.stop(f"Failed to parse integer port from '{PORT_INFO}'!")
    srvr = server.Server('', port)  # don't care about host.
    print(registration_request)
    try:
        srvr.run()
        # utils.stop(f"Server start exception: {srvr.lastErr}")
    except:
        pass

    print(f" key length:  {len(key.export_key())}!!!!2222222222")
    srvr.handleRegistrationRequest(srvr.conn, Registaion_Request)
    srvr.handleSendFileRequest(srvr.conn, SendFileReq)
    print(public_key_req)
    srvr.handlePublicKeyRequest(srvr.conn, publicKeyReq)