# PROGS = gpio-int.c
# COMMON_SRC = interrupts-vec-asm.S

PROGS = scope.c
COMMON_SRC = scope-int.S

# if you need to override the install
LAST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | tail -n 1)
BOOTLOADER = my-install --device $(LAST_DEVICE)

CFLAGS = -I../test-gen-fancy
OPT_LEVEL = -Ofast
OPT_LEVEL = -O3

# automatically run the code
RUN = 1

include $(CS240LX_2024_PATH)/libpi/mk/Makefile.template-fixed
