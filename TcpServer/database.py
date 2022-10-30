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
        except Exception as e:
            log.exception(f'database execute: {e}')
        conn.close()
        return results

    # check if it can be made name or id
    def touch(self,nameOrId):
        """ updating last seen of a client to the time
        last request was received from it """
        results = self.execute(f""" UPDATE {Database.SERVER} 
                                    SET LastSeen = {str(datetime.now())} 
                                    WHERE ID OR NAME = ?;""", [nameOrId])


    def initialize(self):
        # creating the clients table
        query = f"""CREATE TABLE {Database.SERVER}(
                ID CHAR(16) NOT NULL PRIMARY KEY,
                Name CHAR(255) NOT NULL,
                PublicKey CHAR(160),
                LastSeen DATE,
                AesKey CHAR(16));"""
        self.execute(query,[])
        # creating files table
        query = f"""CREATE TABLE {Database.FILES}(
                ID CHAR(16) NOT NULL PRIMARY KEY,
                FileName CHAR(255) NOT NULL,
                PathName CHAR(255),
                Varified INTEGER);""" # boolean if the crc verified
        self.execute(query,[])

    def addClient(self, name):
        """ create a new client instance with name and uuid and store in DB"""
        client = client_info.Client(name, uuid.uuid4().hex, str(datetime.now()))
        return self.execute(f"INSERT INTO {Database.SERVER}(Name ,ID, LastSeen) VALUES (?, ?, ?)",
                            [client.name, client.id, client.lastSeen], True, True)

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
                                WHERE ID = ? ;""" , [clientID, publicKey])
        return results


    def getClientIdByName(self,name):
        """returns client's id corresponding to the name """
        results = self.execute(f""" SELECT ID FROM {Database.SERVER}
                                    WHERE Name = ?; """,[name])
        return results

    def setClientsSymmetricKey(self, clientID):
        aes_key = keys_handler.GenerateAesKey()
        results = self.execute(f""" UPDATE {Database.SERVER} SET AesKey = ?
                                WHERE ID = ? ;""" , [clientID, aes_key])
        return results

    def getClientsSymmetricKey(self, clientID):

        results = self.execute(f""" SELECT AesKey FROM {Database.SERVER}
                                    WHERE ID = ?; """,[clientID])
        return results

    def __str__(self):
        results = self.execute(f"""SELECT * FROM {Database.SERVER}""",[])
        print(results)
        results = self.execute(f"""SELECT * FROM {Database.FILES}""",[])
        print(results)