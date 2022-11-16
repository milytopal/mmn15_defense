import os
from pathlib import Path
import crc
import logging as log


class File:
    """ Represents a file in the database """
    def __init__(self, clientId = b'', fileName = b'', fileContent = b''):
        try:
            self.filename = fileName
            self.clientId = clientId
            self.path = self.getClientsDir(self.clientId)
            self.crc = 0xFFFFFF
            self.validated = False
            self.saveFile(self.filename,fileContent, self.path)
        except Exception as e:
            self.filename = b''
            self.clientId = b''
            self.path = ''
            log.exception(f"failed to store file: {e}")

    def getClientsDir(self, clientId):
        """ get Clients directory where files are saved to, if the client doesnt
        have a directory a new directory will be crated """
        try:
            directory = str(clientId)
            if os.path.isdir(directory):
                curr_dir = os.path.realpath(directory)
                return curr_dir
            else:
                curr_dir = Path(os.path.realpath(__file__))
                parent_dir = curr_dir.parent.absolute()
                new_path = os.path.join(parent_dir, directory)
                os.mkdir(new_path)
                return new_path
        except Exception as e:
            log.exception(f"Failed to create a directory: {e}")
            return None

    def validate(self, validated):
        """ Store validated result of the file """
        self.validated = validated

    def saveFile(self,filename, content, dir):
        try:
            if filename and dir and os.path.isdir(dir):
                if type(filename) == bytes:
                    filename = bytes.decode(filename, 'utf-8')
                file_in_dir = os.path.join(dir, filename)
                with open(rf'{file_in_dir}','wb') as fp:
                    fp.write(content)
                    fp.close()
                    print(f"new file saved:    {file_in_dir}")
        except Exception as e:
            log.error(f"failed to write file: {e}")

    def getContent(self):
        buff = b""
        try:
            if self.filename and self.path and os.path.exists(self.path):
                if type(self.filename) == bytes:
                    filename = bytes.decode(self.filename, 'utf-8')
                file_in_dir = os.path.join(self.path, filename)
                with open(rf'{file_in_dir}', 'rb') as fp:
                    buff = fp.read()
                    fp.close()
            else:
                log.error(f"failed to find file: {self.filename} in dir: {self.path}")
            return buff
        except Exception as e:
            log.error(f"failed to write file: {e}")


class FileHandler:
    """ FileHandler can be created as a general handler or as a specific file handler"""
    def __init__(self, file = None):
        self.directory = ""
        self.file = file

    def update(self,file):
        if type(file) is File:
            self.file = file
        else:
            self.file = None
    def digestCrc(self):
        fileData = self.file.getContent()
        calc = crc.crc32()
        calc.update(fileData)
        self.file.crc = calc.digest()  # fill crc