
import sys
import os
import json

request_name = r'Bootstrap_Request.json'
expected_name = r'Expected_Request.json'

with open(request_name, "r") as request_file:
    requested_data = json.load(request_file)

with open(expected_name, "r") as expected_file:
    expected_data = json.load(expected_file)

res = all((requested_data.get(k) == v for k, v in expected_data.items()))
if res:
    res = all((expected_data.get(k) == v for k, v in requested_data.items()))
print(res)
