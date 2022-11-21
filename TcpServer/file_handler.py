import os
from pathlib import Path
from crccheck.crc import Crc32
import logging as log


class File:
    """ Represents a file in the database """
    def __init__(self, clientId = b'', fileName = b'', fileContent = b''):
        try:
            if type(fileName) == bytes:
                self.filename = bytes.decode(fileName, 'utf-8')
            else:
                self.filename = str(fileName)
            self.clientId = (clientId)
            self.parent_path = self.getClientsDir(self.clientId)
            self.full_path = os.path.join(self.parent_path, self.filename)
            self.crc = 0xFFFFFF
            self.verified = False
            self.content = fileContent

        except Exception as e:
            self.filename = b''
            self.clientId = b''
            self.path = ''
            log.exception(f"failed to store file: {e}")

    def getClientsDir(self, clientId):
        """ get Clients directory where files are saved to, if the client doesnt
        have a directory a new directory will be crated """
        try:
            directory = str(clientId.hex())
            if os.path.isdir(directory):
                # if clients directory already exists return only path to dir
                curr_dir = os.path.realpath(directory)
                return curr_dir
            else:
                # clients directory doesn't exist - creating new one
                curr_dir = Path(os.path.realpath(__file__))
                parent_dir = curr_dir.parent.absolute()
                new_path = os.path.join(parent_dir, directory)
                os.mkdir(new_path)
                return new_path
        except Exception as e:
            log.exception(f"Failed to create a directory: {e}")
            return None


    def saveFile(self,filename = b'', content = b'', dir = ''):
        """ saves file into servers filesystem"""
        try:
            if self:
                with open(rf'{self.full_path}','wb') as fp:
                    fp.write(self.content)
                    fp.close()
                    print(f"content of: {self.full_path} updated")
                    return True
            if filename and dir and os.path.isdir(dir):
                if type(filename) == bytes:
                    filename = bytes.decode(filename, 'utf-8')
                file_in_dir = os.path.join(dir, filename)
                with open(rf'{self.full_path}','wb') as fp:
                    fp.write(content)
                    fp.close()
                    print(f"new file saved:    {file_in_dir}")
                    return True
        except Exception as e:
            log.error(f"failed to write file: {e}")
            return False


class FileHandler:
    """ FileHandler manages files and directory as well as crc calculations of them.
     the files are managed in a dictionary where the key is the clients fullpath with the file name"""

    fileDict = {}   # creating file dictionary as a global
    def __init__(self, file = None):
        self.directory = ""
        self.file = file

    def newFile(self, clientId = b'', fileName = b'', fileContent = b''):
        """ upon creation of a new file in database we create its instance for managing purpeses in the handler
        the files are managed in a dictionary where the key is the clients fullpath with the file name """
        file = File(clientId, fileName, fileContent)
        #adding file to dictinarry -
        # the full path is the unigue part, if file exists it will be overwritten
        self.fileDict[file.full_path] = file
        self.digestCrc(file)
        return file


    def updateFile(self,file):
        """ if a client sent a file with the same name it will be overwritten after validation, therefor
        we save the content for further handeling """
        if type(file) is File:
            if self.fileDict.get(file.full_path):
                self.fileDict[file.full_path].content = file.content
                if self.fileDict[file.full_path].verified:      # file has been already saved to file system
                    return File.saveFile(self.fileDict[file.full_path])

            self.file = file
        else:
            self.file = None
    def digestCrc(self, file):
        """ calculate crc upon insertion """
        fileData = self.fileDict[file.full_path].content
        crc = Crc32.calc(fileData)
        self.fileDict[file.full_path].crc = crc  # fill crc

    def verified(self, file, verified):
        """ update verification status, if verified store the file in filesystem """
        if type(file) is File:
            if self.fileDict.get(file.full_path):
                self.fileDict[file.full_path].verified = verified
                if self.fileDict[file.full_path].verified:
                    File.saveFile(self.fileDict[file.full_path])
                return self.fileDict[file.full_path]
            return None
        return None

    def getFileInstance(self, clientID, fileName):
        """ returns the file instance that store all of the information about the file"""
        directory = str(clientID.hex())
        if os.path.isdir(directory):
            # if clients directory exists
            curr_dir = os.path.realpath(directory)
            if type(fileName) == bytes:
                fileName = bytes.decode(fileName, 'utf-8')
            full_path = os.path.join(curr_dir, fileName)
            return self.fileDict.get(full_path)
        log.error("client doesnt have directory!")
        return None