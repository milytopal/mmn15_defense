import protocol
import uuid


class Client:
    """ Represents a client in the database """
    def __init__(self, name,lastSeen):
        self.name = name
        self.id = uuid.uuid4().bytes_le        # Unique client ID, 16 bytes.
        self.publicKey = b""
        self.AesKey = b""
        self.files = []
        self.directory = b""
        self.lastSeen = lastSeen

    def validate(self):
        """ Validate Client attributes according to the requirements """
        if not self.id or len(self.id) != protocol.CLIENT_ID_SIZE:
            return False
        if not self.name or len(self.name) >= protocol.CLIENT_NAME_SIZE:
            return False
        if not self.publicKey or len(self.publicKey) != protocol.PUBLIC_KEY_SIZE:
            return False
        return True

