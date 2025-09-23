# list out the driver program source 
PROGS = prog.c

include ../defs.mk
LAST_DEVICE=/dev/$$(ls /dev | grep $(USB_DEV) | tail -n 1)
BOOTLOADER = my-install --device $(LAST_DEVICE)

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1
CFLAGS += -g -I../

include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed
