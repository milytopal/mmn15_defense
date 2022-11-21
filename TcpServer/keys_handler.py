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
        # Encrypt the session key with the public RSA key
        cipher_rsa = AES.new(aes_key ,AES.MODE_CBC)
        print( "Generated AES key: " + str(cipher_rsa.IV))
        return cipher_rsa.IV
    except:
        pass

def decrypt(ciphertext, key, content_size):
    """decrypt given content using AES key"""
    try:
        iv_zeros = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        iv = bytearray(iv_zeros)
        decrypt_cipher = AES.new(key, AES.MODE_CBC, iv)
        plain_text = decrypt_cipher.decrypt(ciphertext)
        decrypted_content = plain_text[:content_size]
        return decrypted_content
    except Exception as e:
        print(e)
        return None


def encryptAesKeyWithRsaPublic(data, key):
    """ encrypt given content using RSA key, in this case only the AES key is encrypted
    with the RSA public key from client"""
    try:
        key = bytes.decode(key, 'utf-8')
    except:
        pass
    try:
        RSA.import_key(key)
        recipient_key = RSA.importKey(key, 'DER')
        cipher_rsa = PKCS1_OAEP.new(recipient_key)
        enc_session_key = cipher_rsa.encrypt(data)
        return enc_session_key
    except Exception as e:
        log.error(f"{e}")
        pass