# list out the driver program source 
PROGS = scope.c
COMMON_SRC = scope-asm.S

LAST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | tail -n 1)
BOOTLOADER = my-install --device $(LAST_DEVICE)

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1
CFLAGS += -O3

include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed

scope.txt: objs/scope.elf
	$(OD) -S $< > $@
