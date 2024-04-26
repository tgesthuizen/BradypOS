FROM debian:stable
RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get -y install gcc-arm-none-eabi binutils-arm-none-eabi cmake ninja-build genromfs
RUN apt-get -y clean
RUN useradd -ms /bin/bash agent
USER agent
WORKDIR /home/agent
