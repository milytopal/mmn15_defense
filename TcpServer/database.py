import protocol
import client_info
import sqlite3
import logging as log
import file_handler
import keys_handler
import uuid
from datetime import datetime


class Database:
    SERVER = 'server'
    FILES = 'files'

    def __init__(self, name):
        self.name = name

    # connectivity database
    def connect(self):
        conn = sqlite3.connect(self.name)  # doesn't raise exception.
        conn.text_factory = bytes
        return conn

    def execute(self, query, args, commit=False, push_to_end=False):
        """ Given a query and args, execute query, and return the results. """
        results = None
        conn = self.connect()
        try:
            cur = conn.cursor()
            cur.execute(query, args)
            if commit:
                conn.commit()
                results = True
            else:
                results = cur.fetchall()
            if push_to_end:
                results = cur.lastrowid  # todo: check if needed
            conn.commit()
        except Exception as e:
            log.error(f'database execute: {e}')
        conn.close()
        return results

    # check if it can be made name or id
    def touch(self,nameOrId):
        """ updating last seen of a client to the time
        last request was received from it """
        try:
            results = self.execute(f""" UPDATE {Database.SERVER} 
                                    SET LastSeen = ? 
                                    WHERE ID OR NAME = ?;""",[str(datetime.now()), nameOrId])

        except Exception as e:
            log.error(f"{e}")
            pass

    def initialize(self):
        # creating the clients table
        conn = self.connect()
        cur = conn.cursor()
        try:
            query = f"""CREATE TABLE {Database.SERVER}(
                    ID CHAR(16) NOT NULL PRIMARY KEY,
                    Name CHAR(255) NOT NULL,
                    PublicKey CHAR(160),
                    LastSeen DATE,
                    AesKey CHAR(16));"""
            cur.execute(query)
            conn.commit()
        except Exception as e:
            pass  # table might exist already
        try:    # creating files table
            query = f"""CREATE TABLE {Database.FILES}(
                    ID CHAR(16) NOT NULL,
                    FileName CHAR(255) NOT NULL,
                    PathName CHAR(255) NOT NULL PRIMARY KEY,
                    Varified INTEGER);""" # boolean if the crc verified
            cur.execute(query)
            conn.commit()
        except Exception as e:
            pass        # table might exist already
        conn.close()
    def addClient(self, name):
        """ create a new client instance with name and uuid and store in DB"""
        id =  uuid.uuid4().bytes_le.hex()       # generating unique id for client
        client = client_info.Client(name, id, str(datetime.now()))            # id is stored as bytes
        commited = self.execute(f"INSERT INTO {Database.SERVER}(ID, Name, PublicKey, LastSeen, AesKey) VALUES (?, ?,NULL, ?, NULL)",
                            [client.id, client.name, client.lastSeen], True)
        if commited:
            return client
        else:
            return None

    def storeFile(self,clientId, fileName, content):
        """ create a new client instance with name and uuid and store in DB"""
        try:
            file = file_handler.File(clientId, fileName, content)            # id is stored as bytes
            if file is not None:
                commited = self.execute(f"INSERT INTO {Database.FILES}(ID, FileName, PathName, Varified) VALUES (?, ?, ?, FALSE)",
                                [file.clientId, file.filename, file.path], True)
                if commited:
                    return file
            else:
                return None
        except Exception as e:
            log.error(f"{e}")
            pass

    def clientExists(self, clientId):
        try:
            results = self.execute(f""" SELECT Name FROM {Database.SERVER}
                                        WHERE ID = ?; """,[clientId])
            if not results:
                return False
            return True
        except:
            log.error("Failed to Connect to Database")


    def getClientsPublicKey(self,clientID):
        results = self.execute(f""" Select PublicKey FROM {Database.SERVER}
                                    WHERE ID = ?; """,[clientID])
        return results

    def setClientsPublicKey(self, clientID, publicKey): # todo: implement correctly
        """ given a client id, store the public key, generate the AES key and return it to client. """
        if len(publicKey) is not protocol.PUBLIC_KEY_SIZE:
            log.info(f"setClientsPublicKey: Public key size is invalid.")
            return None # public key invalid
        self.touch(clientID)
        results = self.execute(f""" UPDATE {Database.SERVER} SET PublicKey = ?
                                WHERE ID = ? ;""" , [clientID, publicKey], True)
        return results


    def getClientIdByName(self,name):
        """returns client's id corresponding to the name """
        results = self.execute(f""" SELECT ID FROM {Database.SERVER}
                                    WHERE Name = ?; """,[name])
        return results

    def setClientsSymmetricKey(self, clientID):
        aes_key = keys_handler.GenerateAesKey()
        if self.execute(f""" UPDATE {Database.SERVER} SET AesKey = ?
                                WHERE ID = ? ;""" , [clientID, aes_key], True):
            return aes_key
        return None

    def getClientsSymmetricKey(self, clientID):

        results = self.execute(f""" SELECT AesKey FROM {Database.SERVER}
                                    WHERE ID = ?; """,[clientID])
        return results

    

    def __str__(self):
        results = self.execute(f"""SELECT * FROM {Database.SERVER}""",[])
        print(results)
        results = self.execute(f"""SELECT * FROM {Database.FILES}""",[])
        print(results)

    def clientUsernameExists(self, name):
        pass