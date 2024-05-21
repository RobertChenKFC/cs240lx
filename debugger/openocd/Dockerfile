FROM ubuntu:latest
WORKDIR /openocd
RUN apt update
RUN apt install -y git make pkg-config autoconf libtool libusb-1.0-0 libusb-1.0-0-dev wget
RUN git clone https://github.com/raspberrypi/openocd.git
RUN wget https://github.com/tttapa/docker-arm-cross-toolchain/releases/latest/download/x-tools-armv6-rpi-linux-gnueabihf.tar.xz
RUN apt install xz-utils
RUN tar xvf x-tools-armv6-rpi-linux-gnueabihf.tar.xz
RUN apt install -y vim
