
import utils
import server
from argparse import ArgumentParser

# Initialize instance of an argument parser
parser = ArgumentParser(description='Multi-threaded TCP Server')

# Add optional argument, with given default values if user gives no arg
parser.add_argument('-p', '--port', default=1234, type=int, help='Port over which to connect')

# Get the arguments
args = parser.parse_args()

if __name__ == '__main__':
    PORT_INFO = "port.info"
    port = utils.init_port_config(PORT_INFO)
    if port is None:
        utils.stop(f"Failed to parse integer port from '{PORT_INFO}'!")
    srvr = server.Server('', port)  # don't care about host.
    if not srvr.run():
        utils.stop(f"Server start exception: {srvr.lastErr}")

