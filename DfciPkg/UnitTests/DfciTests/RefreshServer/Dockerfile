
# Dfci Refresh from Network Test Server
#
# To build the Dfci test server docker image:
#   docker build . -t dfci_server
#
# To run the docker image:
#   docker run -p 80:80 -p 443:443 -it dfci_server
#
# To run the docker image but run a bash shell instead of the web server:
#   docker run -p 80:80 -p 443:443 -it dfci_server bash
#
# To test the web server from the docker host:
#   curl -iv --cert DFCI_HTTPS.cer localhost:443
#
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent

FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV WEB_DIR /srv/dfci_refresh_server/src
RUN apt update \
  && apt install python3 -y \
  && apt install python3-pip -y \
  && rm -rf /var/lib/apt/lists/* \
  && pip3 install cherrypy flask

COPY Src/ ${WEB_DIR}/
WORKDIR ${WEB_DIR}
CMD ["python3", "server.py"]

EXPOSE 80
EXPOSE 443
