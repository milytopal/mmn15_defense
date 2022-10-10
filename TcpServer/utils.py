from os import path


# returns file if file exists and readable
def init_port_config(filename):
    try:
        if not path.isfile(filename):
            print("Invalid filename provided.")
            return None
        configfile = open(filename,"r")
        if not configfile.readable():
            print(f"Unable to read file {filename}.")
            return None
        # read config line and remove leading and trailing spaces
        # if there is more than one line it will be ignored
        line = configfile.readline().strip()
        port = None
        # first argument is expected to be IP address and second to be port number
        try:
            port = int(line)
        except ValueError:
            print("invalid port number, please check configuration")
        except IndexError:
            print("missing port number")
        except:
            print("unknown error occurred while parsing configuration")
        # if no error occurred
        return port
    except (IOError, ValueError) as e:
        print(e)
        return None

def stop(err):
    """ Print err and stop script execution """
    print("\nFatal Error!", err, "the server will stop.", sep="\n")
    exit(1)