FROM node:latest AS builder
RUN mkdir /workdir
RUN chown node:node /workdir
WORKDIR /workdir
USER node
RUN wget -O rp2040js.tar.gz "https://github.com/wokwi/rp2040js/archive/refs/tags/v1.0.1.tar.gz"
RUN tar -xf rp2040js.tar.gz
WORKDIR /workdir/rp2040js-1.0.1
RUN npm install
ENTRYPOINT ["npm", "start"]
