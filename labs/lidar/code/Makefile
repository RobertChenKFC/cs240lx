# list out the driver program source 
PROGS = main.c
COMMON_SRC = hw-uart.c
STAFF_OBJS = sw-uart.o

LAST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | tail -n 1)
BOOTLOADER = my-install --device $(LAST_DEVICE)

# set RUN = 1 if you want the code to automatically run after building.
RUN = 0
CFLAGS += -O3
VISUALIZE = ./visualize.py

include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed

BIN = $(PROGS:.c=.bin)
.PHONY: visualize
.DEFAULT_GOAL := visualize
visualize: $(BIN)
	$(VISUALIZE)
	# $(BOOTLOADER) $(BIN)

