// weak test of malloc/free: checks that subsequent allocations are
// <sizeof (void *)> bytes from each other.
#include "test-interface.h"
#include "ckalloc.h"

union align {
        double d;
        void *p;
        void (*fp)(void);
};

typedef union header { /* block header */
    struct {
            union header *ptr; /* next block if on free list */
            unsigned size; /* size of this block */
    } s;
    union align x; /* force alignment of blocks */
} Header;

#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

void notmain(void) {

    char *p0 = ckalloc(1);
    unsigned n = roundup(sizeof(hdr_t) + sizeof(Header) + 1, sizeof(Header));

    // DEBUG
    // panic("n=%d\n", n);
    int ntests = 10;

    output("malloc(1) = %p\n", p0);
    for(int i = 0; i < ntests; i++) {
        char *p1 = ckalloc(1);
        output("ckalloc(1) = %p\n", p1);

        assert(p0>p1);
        if(p1 + n * (i + 1) != p0)
            panic("p0=%p, p1=%p, expected diff=%u, actual=%u\n", p0,p1,n, p0-p1);
        else
            output("passed iter %d\n", i);
    }
    trace("success: malloc/free appear to work on %d tests!\n", ntests);
}
