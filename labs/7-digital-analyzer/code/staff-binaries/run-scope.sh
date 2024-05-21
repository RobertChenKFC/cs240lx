#!/bin/bash
devices=$(ls /dev/ | grep cu.usbserial)
last_device=$(echo "$devices" | tail -n 1)
my-install --device /dev/$last_device scope.bin
