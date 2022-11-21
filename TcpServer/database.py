import protocol
import client_info
import sqlite3
import logging as log
from file_handler import File, FileHandler
import keys_handler
from datetime import datetime


class Database:
    SERVER = 'server'
    FILES = 'files'

    def __init__(self, name):
        self.name = name
        self.fileHandler = FileHandler()

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
                                    WHERE ID OR NAME = ?;""",[str(datetime.now()), nameOrId], True)

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
                    FileName CHAR(255) NOT NULL PRIMARY KEY,
                    PathName CHAR(255) NOT NULL,
                    Varified INTEGER);""" # boolean if the crc verified
            cur.execute(query)
            conn.commit()
        except Exception as e:
            pass        # table might exist already
        conn.close()
    def addClient(self, name):
        """ create a new client instance with name and uuid and store in DB"""
            # generating unique id for client
        client = client_info.Client(name, str(datetime.now()))            # id is stored as bytes
        commited = self.execute(f"INSERT INTO {Database.SERVER}(ID, Name, PublicKey, LastSeen, AesKey) VALUES (?, ?,NULL, ?, NULL)",
                            [client.id, client.name, client.lastSeen], True)
        if commited:
            return client
        else:
            return None

    def fileExists(self, clientId, fileName):
        try:
            results = self.execute(f""" SELECT * FROM {Database.FILES}
                                          WHERE ID = (?) AND FileName = (?); """, [clientId, fileName])
            if not results:
                return False
            return True
        except:
            log.error("Failed to Connect to Database")
            return False


    def updateVerification(self, clientId, fileName, verified):
        """ update verification and store file in file system """
        try:
            if self.fileExists(clientId, fileName):
                self.touch(clientId)
                # if file exists only update the varified status
                file = self.fileHandler.getFileInstance(clientId,fileName)
                self.fileHandler.verified(file, verified)
                commited = self.execute(
                    f"""UPDATE {Database.FILES} SET Varified = (?) 
                     WHERE FileName = (?)  AND ID = (?) """,
                    [verified, file.filename, file.clientId], True)
                return True
            else:
                log.error("file doesnt exists")
                return False
        except Exception as e:
            log.error(f"failed to update: {e}")
            return False

    def getFile(self, clientId, fileName):
        try:
            if self.fileExists(clientId, fileName):
                file = self.fileHandler.getFileInstance(clientId,fileName)
                if file:
                    return file
                else:
                    return None
            else:
                log.error("file doesnt exists")
                return None
        except Exception as e:
            log.error(f"DB return with error: {e}")
            return None


    def storeFileinDB(self, clientId, fileName, newContent):
        """ store a recieved file in db' if file already exists we assume the content is new and
        only update the content, otherwise new file instance will be created and store in db (not in filesystem yet)"""
        try:
            file = self.fileHandler.newFile(clientId, fileName, newContent)
            if self.fileExists(clientId, fileName):
                # if file exists update the verified status to false without storing the new one in filesystem
                commited = self.execute(
                f"""UPDATE {Database.FILES} SET Varified = FALSE 
                 WHERE FileName = (?)  AND ID = (?)  """,
                [file.filename, file.clientId ], True)
                if commited:
                    self.touch(clientId)
                    return file
            else:
                # new file
                file = self.fileHandler.newFile(clientId,fileName, newContent)
                if file:
                    commited = self.execute(f"INSERT INTO {Database.FILES}(ID, FileName, PathName, Varified) VALUES (?, ?, ?, FALSE)",
                                    [file.clientId, file.filename, file.parent_path], True)
                    if commited:
                        self.touch(clientId)
                        return file
                    else:
                        log.error("failed to commit file")
                        return None
                else:
                    return None
        except Exception as e:
            log.error(f"{e}")
            pass

    def clientExists(self, clientId):
        try:
            results = self.execute(f""" SELECT Name FROM {Database.SERVER}
                                        WHERE ID = (?) ; """,[clientId])
            if not results:
                return False
            return True
        except:
            log.error("Failed to Connect to Database")


    def getClientsPublicKey(self,clientID):
        results = self.execute(f""" Select PublicKey FROM {Database.SERVER}
                                    WHERE ID = (?) ; """,[clientID])
        return results[0][0]

    def setClientsPublicKey(self, clientID, publicKey): # todo: implement correctly
        """ given a client id, store the public key, generate the AES key and return it to client. """
        if len(publicKey) is not protocol.PUBLIC_KEY_SIZE:
            log.info(f"setClientsPublicKey: Public key size is invalid.")
            return None # public key invalid
        self.touch(clientID)
        results = self.execute(f""" UPDATE {Database.SERVER} SET PublicKey = ?
                                WHERE ID = (?)  ;""" , [publicKey, clientID], True)
        return results


    def getClientNameById(self,clientId):
        """returns client's id corresponding to the name """
        try:
            results = self.execute(f""" SELECT Name FROM {Database.SERVER}
                                    WHERE ID = (?) ; """,[clientId])
            if results:
                return results[0][0]
            return None
        except:
            return None

    def setClientsSymmetricKey(self, clientID):
        """ stores symmetric key in db"""
        aes_key = keys_handler.GenerateAesKey()
        if self.execute(f""" UPDATE {Database.SERVER} SET AesKey = ?
                                WHERE ID = (?) ;""" , [aes_key, clientID], True):
            return aes_key
        return None

    def getClientsSymmetricKey(self, clientID):
        """ selects clients symmetric key """
        results = self.execute(f""" SELECT AesKey FROM {Database.SERVER}
                                    WHERE ID = (?) ; """,[clientID])
        if results[0]:
            #results = bytes.fromhex(results[0][0])
            return results[0][0]
        else:
            return None

    def clientUsernameExists(self, clientsName):
        try:
            results = self.execute(f""" SELECT * FROM {Database.SERVER}
                                                      WHERE Name = (?); """, [clientsName])
            if results:
                return True
            else:
                return False
        except Exception as e :
            log.error(f"error occurred: {e}")
            pass
