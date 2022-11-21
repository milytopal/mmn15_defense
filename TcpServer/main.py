import utils
import server
import logging as log

PORT_INFO = "port.info"
logger = log.getLogger(__name__)
FORMAT = "[%(filename)s:%(lineno)s - %(funcName)10s() ] %(message)s"
log.basicConfig(format=FORMAT)
logger.setLevel(log.DEBUG)

if __name__ == '__main__':
    port = utils.init_port_config(PORT_INFO)
    if port is None:
        utils.stop(f"Failed to parse integer port from '{PORT_INFO}'!")
    srvr = server.Server('', port)  # don't care about host.
    try:
        srvr.run()
    except Exception as e:
        log.error(f"Server Main Loop exited with exception: {e}")
        pass
