#include "rpi.h"

void notmain(void) {
    // print out the string in the header.
    extern uint32_t __hdr_start__[];
    // figure out where it points to!
    const char *header_string = (const char*)__hdr_start__;

    assert(header_string);
    printk("<%s>\n", header_string);
    printk("success!\n");
    clean_reboot();
}
