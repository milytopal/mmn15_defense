
import logging
import sqlite3
import protocol

class ClientHandler:
    """ Represents every client registered to the server """

    def __init__(self, cid, cname, public_key, last_seen):
        self.ID = bytes.fromhex(cid)  # Unique client ID, 16 bytes.
        self.Name = cname  # Client's name, null terminated ascii string, 255 bytes.
        self.PublicKey = public_key  # Client's public key, 160 bytes.
        self.LastSeen = last_seen  # The Date & time of client's last request.

    def validate(self):
        """ Validate Client attributes according to the requirements """
        if not self.ID or len(self.ID) != protocol.CLIENT_ID_SIZE:
            return False
        if not self.Name or len(self.Name) >= protocol.NAME_SIZE:
            return False
        if not self.PublicKey or len(self.PublicKey) != protocol.PUBLIC_KEY_SIZE:
            return False
        if not self.LastSeen:
            return False
        return True

