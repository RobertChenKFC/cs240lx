# list out the driver program source 
PROGS = scope.c

FIRST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | head -n 1)
LAST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | tail -n 1)
BOOTLOADER = my-install --device $(LAST_DEVICE)

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1
CFLAGS += -O3 -I../

include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed
