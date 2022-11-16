import uuid
import protocol
import logging as log
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES, PKCS1_OAEP
from Crypto import Random



def GenerateAesKey():
    try:
        # generate random Aes key
        aes_key = Random.get_random_bytes(protocol.SYMMETRIC_KEY_SIZE)
        # aes_key = Random.new().read(protocol.SYMMETRIC_KEY_SIZE)
        # Encrypt the session key with the public RSA key
        cipher_rsa = AES.new(aes_key ,AES.MODE_CBC)
        return cipher_rsa.IV
    except:
        pass

def decrypt(ciphertext, key):
    try:
        return AES.new(key, AES.MODE_CBC).decrypt(ciphertext, key).decode('utf-8')
    except:
        return None

def encryptAesKeyWithRsaPublic(data, key):
    #key = bytes.decode(key, 'utf-8')
    print(f"key len: {len(key)}")
    try:
        RSA.import_key(key)
        recipient_key = RSA.importKey(key, 'DER')
        cipher_rsa = PKCS1_OAEP.new(recipient_key)
        enc_session_key = cipher_rsa.encrypt(data)
        return enc_session_key
    except Exception as e:
        log.error(f"{e}")
        pass