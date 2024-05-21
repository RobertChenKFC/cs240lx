#!/bin/bash
devices=$(ls /dev/ | grep cu.usbserial)
first_device=$(echo "$devices" | head -n 1)
my-install --device /dev/$first_device test-gen.bin
