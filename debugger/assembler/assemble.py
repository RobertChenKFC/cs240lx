#!/usr/bin/env python3

import os

while True:
    inst = input("Enter instruction: ")
    os.system(f'echo "{inst}" > main.S')
    os.system("arm-none-eabi-gcc -c -o main.o main.S")
    os.system("arm-none-eabi-objcopy main.o -O binary main.bin")
    with open(f"main.bin", "rb") as infile:
        c = 0
        x = 0
        for b in infile.read():
            x |= int(b) << c
            c += 8
            if c == 32:
                print(hex(x))
                c = 0
