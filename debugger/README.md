# JTAG Debugger For Pi Zero using Another Pi Zero
This uses a Pi Zero to bitbang the JTAG protocol to another Pi Zero. Most of the functionality is implemented in the following files/directories:
* `jtag.h`/`jtag.c`: the library to bitbang JTAG to read/write the registers and memory of another Pi Zero.
* `probe/`: the program that will run on the Pi Zero to act as a JTAG debugger and receives/responds to commands from the host computer using UART.
* `bootloader/`: the bootloader program that runs on the host computer to communicate between the Pi Zero and the GDB server.
## Instructions
1. Connect pins 23 through 27 between the two Pi Zeros (so five pins should be connected).
2. Ensure the Pi Zero being debugged has the JTAG pins exposed. The easiest way is to add the line `enable_jtag_gpio=1` to `config.txt` in the SD card of the Pi Zero being debugged.
3. Run `make` under `bootloader` to start the JTAG debugger.
4. Start GDB and connect to port 3333.
