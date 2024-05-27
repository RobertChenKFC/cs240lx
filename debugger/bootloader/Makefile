PROGS = jtag-install.c
COMMON_SRC = put-code.c put-get.c
CFLAGS += -I2-pi-side -I..

BOOTLOADER = $(PROGS:.c=)
PROBE_PATH = ../probe/
PROBE = $(PROBE_PATH)probe.bin

FIRST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | head -n 1)
LAST_DEVICE=/dev/$$(ls /dev | grep cu.usbserial | tail -n 1)
test: $(BOOTLOADER) $(PROBE)
	./jtag-install --device $(FIRST_DEVICE) $(PROBE)

.PHONY: $(PROBE)
$(PROBE):
	make -C $(PROBE_PATH) $(notdir $(PROBE))

include $(CS140E_2024_PATH)/libunix/mk/Makefile.unix

trace:
	./my-install --trace-control hello.bin

trace-all:
	./my-install --trace-all hello.bin

.PHONEY: trace trace-all
