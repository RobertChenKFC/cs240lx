#!/bin/bash
FIRST_DEVICE=/dev/$(ls /dev | grep cu.usbserial | head -n 1)
screen $FIRST_DEVICE 115200
