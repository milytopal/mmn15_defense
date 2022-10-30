import uuid
import protocol
import logging as log
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES
from Crypto import Random

def GenerateAesKey():
    aes_key = Random.get_random_bytes(protocol.SYMMETRIC_KEY_SIZE)
    # Encrypt the session key with the public RSA key
    cipher_rsa = AES.new(aes_key ,AES.MODE_CBC)
    return cipher_rsa


def decrypt(ciphertext, key):
    try:
        return AES.decrypt(ciphertext, key).decode('utf-8')
    except:
        return False

def RSAencrypt(data, key):
    recipient_key = RSA.import_key(key)
    cipher = RSA.new(recipient_key)
    # enc_session_key = cipher.encrypt_and_digest(data)
    ciphertext, tag = cipher.encrypt_and_digest(data)
    return ciphertext