# list out the driver program source 
PROGS = test-gen.c
COMMON_SRC = test-gen-asm.S

# pick one of your attachment points (first or last) consistently.
FIRST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | head -n 1)
BOOTLOADER = my-install --device $(FIRST_DEVICE)

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

CFLAGS += -O3 -I$(CS240LX_2024_PATH)labs/8-pmu/code
include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed

test-gen.txt: objs/test-gen.elf
	$(OD) -S $< > $@

