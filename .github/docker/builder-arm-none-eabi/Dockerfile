FROM debian:stable
RUN apt-get update \
    && apt-get -y upgrade \
    && apt-get install -y --no-install-recommends gcc-arm-none-eabi binutils-arm-none-eabi cmake ninja-build genromfs \
    && apt-get clean
RUN useradd -ms /bin/bash agent
USER agent
WORKDIR /home/agent
