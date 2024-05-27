#!/usr/bin/env python3

import os


while True:
    inst = input("Enter machine code: ")

    inst = int(inst, 16)
    with open("main.bin", "wb") as outfile:
        outfile.write(inst.to_bytes(4, byteorder="little"))

    os.system(
        "arm-none-eabi-objdump -b binary -D -m armv6 main.bin > main.list"
    )
    with open("main.list", "r") as infile:
        for line in infile.read().split("\n"):
            texts = line.split()
            for i, text in enumerate(texts):
                if hex(inst)[2:] in text:
                    print(" ".join(texts[i + 1:]))
