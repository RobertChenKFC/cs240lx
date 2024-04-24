
# PROGS += 0-test-malloc.c
PROGS += 2-test-malloc.c

COMMON_SRC = pi-sbrk.c

# swap these
COMMON_SRC += kr-malloc.c 
# STAFF_OBJS = staff-kr-malloc.o


RUN=1

# DEBUG
UNIX=0

EXCLUDE ?= grep -v simple_boot
ifeq ($(UNIX), 1)
CFLAGS += -DCOMPILE_FOR_UNIX
include ../../../libunix/mk/Makefile.unix
else
include ../../../libpi/mk/Makefile.template-fixed
endif
