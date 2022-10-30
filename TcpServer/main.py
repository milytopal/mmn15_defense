
import utils
import server

PORT_INFO = "port.info"

if __name__ == '__main__':
    port = utils.init_port_config(PORT_INFO)
    if port is None:
        utils.stop(f"Failed to parse integer port from '{PORT_INFO}'!")
    srvr = server.Server('', port)  # don't care about host.
    if not srvr.run():
        utils.stop(f"Server start exception: {srvr.lastErr}")

