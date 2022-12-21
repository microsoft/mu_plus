import configparser
import json
import os
import traceback

from flask import Flask, request, send_file, jsonify

dfci_refresh_server = Flask(__name__)

dfci_refresh_server.config['REQUEST_FOLDER'] = '/srv/dfci_refresh_server/src/Requests'
dfci_refresh_server.config['RESPONSE_FOLDER'] = '/srv/dfci_refresh_server/src/Responses'
DfciTest_Config = '/srv/dfci_refresh_server/src/DfciTests.ini'


def compare_json_files(request_name, expected_name):

    with open(request_name, "r") as request_file:
        requested_data = json.load(request_file)

    with open(expected_name, "r") as expected_file:
        expected_data = json.load(expected_file)

    is_equal = all((requested_data.get(k) == v for k, v in expected_data.items()))
    if is_equal:
        is_equal = all((expected_data.get(k) == v for k, v in requested_data.items()))
    return is_equal


def get_host_name():
    if not os.path.exists(DfciTest_Config):
        raise Exception("Unable to locate test configuration template.")

    config = configparser.ConfigParser()
    config.read(DfciTest_Config)

    return config["DfciTest"]["server_host_name"]


#
#  Default web page for this server to verify that the test DFCI server is functional.
#
@dfci_refresh_server.route('/')
def hello_world():
    try:
        if request.url.startswith('http://'):
            msg = 'Http - Hello, World! DFCI Test Server V 3.0 serving DfciRequests.'
        elif request.url.startswith('https://'):
            msg = 'Https - Hello, World! DFCI Test Server V 3.0 serving DfciRequests.'
        else:
            raise Exception(f'Unsupported format URL {request.url}')
        msg += '\nRequest from '
        msg += request.remote_addr
        return msg
    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 500
        return r


#
#  Force a return of 429 to verify the network stack passes 429 back through UEFI.
#  Intune uses 429 to throttle requests when the server is busy.
#  Checked byt the unit test DfciCheck429.efi.
#
@dfci_refresh_server.route('/return_429')
def return_429():
    try:
        if not request.url.startswith('http://') and not request.url.startswith('https://'):
            raise Exception(f'Unsupported format URL {request.url}')

        msg = 'Too many requests '
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 429
        return r

    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 501
        return r


#
# Get the host name being used for the redirection requests
#
@dfci_refresh_server.route('/GetHostName')
def gethostname():

    try:
        if not request.url.startswith('https://'):
            raise Exception(f'Unsupported format URL {request.url}')

        r = dfci_refresh_server.make_response(get_host_name())
        r.mimetype = 'text/plain'
        r.status_code = 200
        return r
    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 502
        return r


#
# Test recovery bootstrap InTune model.  This is an async request with the response
# telling Dfci where to go to get the payload/
#
@dfci_refresh_server.route('/ztd/unauthenticated/dfci/recovery-bootstrap', methods=['POST'])
def recovery_bootstrap_request():
    #
    # A Ztd bootstrap request comes with a JSON Body:
    #
    #
    try:
        if not request.url.startswith('http://'):
            raise Exception(f'Unsupported format URL {request.url}')

        #
        # Expect a body, and store the body for later verification
        #
        if request.mimetype == 'application/json':
            filename = 'Bootstrap_Request.json'
            pathname = os.path.join(dfci_refresh_server.config['REQUEST_FOLDER'], filename)
            file = open(pathname, 'wb')
            file.write(request.data)
            file.close()

            response = jsonify()
            response.status_code = 202
            #

            location = 'http://' + get_host_name() + '/ztd/unauthenticated/dfci/recovery-bootstrap-status/request-id'
            response.headers['Location'] = location
            return response
        else:
            response = jsonify()
            response.status_code = 406
            return response

    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 504
        return r


#
# Test recovery bootstrap response server.  This test the client to see if the certs
# need updating.  If so, cert update packets are returned If not, returns a "NULL" response.
#
@dfci_refresh_server.route('/ztd/unauthenticated/dfci/recovery-bootstrap-status/request-id', methods=['GET'])
def recovery_bootstrap_status():

    try:
        if not request.url.startswith('http://'):
            raise Exception(f'Unsupported format URL {request.url}')

        request_name = os.path.join(dfci_refresh_server.config['REQUEST_FOLDER'], 'Bootstrap_Request.json')
        expected_name = os.path.join(dfci_refresh_server.config['REQUEST_FOLDER'], 'Expected_Request.json')

        is_equal = compare_json_files(request_name, expected_name)

        if is_equal:
            filename = 'Bootstrap_NULLResponse.json'
        else:
            filename = 'Bootstrap_Response.json'

        pathname = os.path.join(dfci_refresh_server.config['RESPONSE_FOLDER'], filename)
        r = dfci_refresh_server.make_response(send_file(pathname))
        r.headers["Cache-Control"] = "must-revalidate"
        r.headers["Pragma"] = "must-revalidate"
        r.mimetype = 'application/json'
        r.status_code = 200
        return r

    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 505
        return r


#
# Update the device settings etc after the certs are updated.  This is another
# async request where a location header is specified to inform Dfci where to
# retrieve the update packets.
#
@dfci_refresh_server.route('/ztd/unauthenticated/dfci/recovery-packets', methods=['POST'])
def recovery_packets():
    #
    # A Recovery request comes with a JSON Body:
    #
    # The main purpose is to redirect to a different URL to get the packets.
    #
    try:
        if not request.url.startswith('https://'):
            raise Exception(f'Unsupported format URL {request.url}')

        #
        # Expect a body, and store the body for later verification, if necessary
        #
        if request.mimetype == 'application/json':
            filename = 'Recovery_Request.json'
            pathname = os.path.join(dfci_refresh_server.config['REQUEST_FOLDER'], filename)
            file = open(pathname, 'wb')
            file.write(request.data)
            file.close()

            response = jsonify()
            response.status_code = 202
            #
            # return a full URL with https instead of http
            response.autocorrect_location_header = False
            location = 'https://' + get_host_name() + '/ztd/unauthenticated/dfci/recovery-packets-status/request-id'
            response.headers['Location'] = location
            return response
        else:
            response = jsonify()
            response.status_code = 406
            return response

    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 500
        return r


#
# Recovery packet response location.  Return updated settings.
#
@dfci_refresh_server.route('/ztd/unauthenticated/dfci/recovery-packets-status/request-id', methods=['GET'])
def recovery_packets_status():

    try:
        if not request.url.startswith('https://'):
            raise Exception(f'Unsupported format URL {request.url}')

        filename = 'Recovery_Response.json'
        pathname = os.path.join(dfci_refresh_server.config['RESPONSE_FOLDER'], filename)
        r = dfci_refresh_server.make_response(send_file(pathname))
        r.headers["Cache-Control"] = "must-revalidate"
        r.headers["Pragma"] = "must-revalidate"
        r.mimetype = 'dfci_refresh_serverlication/json'
        r.status_code = 200
        return r

    except Exception:
        msg = ''.join(traceback.format_exc())
        r = dfci_refresh_server.make_response(msg)
        r.mimetype = 'text/plain'
        r.status_code = 501
        return r


if __name__ == '__main__':

    dfci_refresh_server.debug = True
    dfci_refresh_server.run()
