# Web Server for testing the Refresh from Network function in DFCI.


import cherrypy
from main import dfci_refresh_server


if __name__ == '__main__':

    # Mount the application
    cherrypy.tree.graft(dfci_refresh_server, "/")

    # Unsubscribe the default server
    cherrypy.server.unsubscribe()
    cherrypy.engine.signals.subscribe()

    cherrypy.config.update({'environment': 'embedded'})

    # Instantiate a new server object
    server_http = cherrypy._cpserver.Server()

    # Configure the server object
    server_http.socket_host = "0.0.0.0"
    server_http.socket_port = 80
    server_http.thread_pool = 30

    server_https = cherrypy._cpserver.Server()

    # Configure the server object
    server_https.ssl_module = 'builtin'
    server_https.socket_host = "0.0.0.0"
    server_https.socket_port = 443
    server_https.thread_pool = 30
    server_https.ssl_certificate = 'ssl/DFCI_HTTPS.pem'
    server_https.ssl_private_key = 'ssl/DFCI_HTTPS.key'

    # Subscribe this server
    server_http.subscribe()
    server_https.subscribe()

    # Start the server engine (Option 1 *and* 2)

    cherrypy.engine.start()

    cherrypy.engine.block()
