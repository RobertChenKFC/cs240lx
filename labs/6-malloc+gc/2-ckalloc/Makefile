PROGS += 5-test-ckalloc.c 
# PROGS += 4-test-ckalloc.c 
# PROGS := $(wildcard [012345]-test*.c)
# PROGS := $(wildcard [1]-test*.c)

COMMON_SRC = pi-sbrk.c

# swap these
# STAFF_OBJS += staff-ckalloc.o
COMMON_SRC += ckalloc.c
# STAFF_OBJS += staff-kr-malloc.o
COMMON_SRC += kr-malloc.c

RUN=1

EXCLUDE ?= grep -v simple_boot
include ../../../libpi/mk/Makefile.robust
