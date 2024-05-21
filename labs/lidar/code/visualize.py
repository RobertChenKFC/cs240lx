#!/usr/bin/env python3

import os
import shutil
import signal
import matplotlib.pyplot as plt
from tkinter import *
from tkinter import messagebox
from math import pi, sin, cos

# Canvas stuff
top = Tk()
width = 600
height = 600
mx = width // 2
my = height // 2
canvas = Canvas(top, bg="white", height=height, width=width)
radius = 5
color = "red"

# Lidar stuff
n_points = 12
n_samples = 10
scale = pi / 18000
intensity_thresh = 150


# DEBUG
cnt = 0
dist_scale = 3

def update():
    # DEBUG
    global cnt

    canvas.delete("all")
    for _ in range(38):
        # DEBUG
        sync = int(input(), 16)
        assert sync == 0xbeef

        s1 = input()
        s2 = input()
        # print(s1, s2)

        start_angle = int(s1, 16) * scale
        end_angle = int(s2, 16) * scale
        end_of_loop = start_angle > end_angle
        if end_of_loop:
            end_angle += 2 * pi

        # DEBUG
        """
        print(start_angle, end_angle)
        cnt += 1
        if cnt > 10:
            break
        """

        for i in range(0, n_points):
            dist = int(input(), 16)
            # intensity = int(input(), 16)

            # DEBUG
            # print(intensity)
            # if intensity < intensity_thresh:
            #     continue

            angle = (
                start_angle * (n_points - i) + end_angle * i
            ) / n_points

            # DEBUG
            # print(dist, angle)
            x = mx + dist_scale * dist * cos(angle)
            y = my - dist_scale * dist * sin(angle)
            canvas.create_oval(
                x - radius, y - radius, x + radius, y + radius, fill=color,
                outline=""
            )

        """
        if end_of_loop:
            break
        """
    top.after(10, update)


r, w = os.pipe()
pid = os.fork()
if pid == 0:
    os.dup2(w, 1)
    os.dup2(w, 2)
    my_install = shutil.which("my-install")
    os.execl(my_install, "my-install", "main.bin")
else:
    os.dup2(r, 0)

    i = 0
    while True:
        s = input()
        print(s)
        if s == "==============================":
            break
        i += 1
        if i > 100:
            exit(0)

    canvas.pack()
    update()
    top.mainloop()
