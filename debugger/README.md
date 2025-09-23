# JTAG Debugger For Pi Zero using Another Pi Zero
This uses a Pi Zero to bitbang the JTAG protocol to another Pi Zero. Most of the functionality is implemented in the following files/directories:
* `jtag.h`/`jtag.c`: the library to bitbang JTAG to read/write the registers and memory of another Pi Zero.
* `probe/`: the program that will run on the Pi Zero to act as a JTAG debugger and receives/responds to commands from the host computer using UART.
* `bootloader/`: the bootloader program that runs on the host computer to communicate between the Pi Zero and the GDB server.
## Instructions
These instructions are for debugging the simple program written in `prog/prog.c`. Once you confirm that the debugger is working, you can easily extend this to debug other programs.

This example is written so that the debugger is run on the *first* device, while the debuggee is run on the *second* device (here, first and second means the order they appear under `/dev/`). However, for simplicity, you can set up the two Pi's in the same way so that it doesn't matter which Pi gets picked as the debuggee.

0. Confirm that the `USB_DEV` variable in `defs.mk` is set according to your host OS
1. Connect pins 23 through 27 and GND between the two Pi Zeros (so 6 pins should be connected).
2. Debuggee: ensure the JTAG pins are exposed. The easiest way is to add the line `enable_jtag_gpio=1` to `config.txt` in the SD card of the debuggee.
3. Debuggee: connect pin 22 to VCC.
4. Debuggee (optional): connect the positive and negative of an LED to pin 12 and GND of the debuggee.
5. Debuggee: run `cd prog/; make`. The LED of the debuggee should start flashing.
6. Debugger: run `cd bootloader; make`. The LED of the debuggee should stop flashing.
7. Run `cd prog/; gdb objs/prog.elf` to launch GDB, and run `target remote :3333` to attach.
8. Have fun :) You should be able to break at a particular line (like `b prog.c:20; c`) and then play with it.
