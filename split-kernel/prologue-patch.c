#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/mman.h>   // mprotect
#include <unistd.h>     // getpagesize
#include <errno.h>


#define __original      __attribute__ ((ms_hook_prologue))


#define create_pair(NAME)                                       \
__attribute__ ((ms_hook_prologue))                              \
void NAME() { printf("%s from split-base\n", #NAME); }          \
void NAME##_hdn() { printf("%s from SPLIT-HARDNED!\n", #NAME); }


create_pair(M0);
create_pair(M1);
create_pair(M2);
create_pair(M3);
create_pair(M4);


const static void *symbols_table[] = { M0, M1, M2, M3, M4 };



// Just load symbols_table address to register RAX
#define load_table_address()    asm("movq %0, %%rax" : : "r" (symbols_table) : "%rax")

// Force a debug breakpoint
#define breakpoint()            asm("int $3")





void hotpatch(void *target, void *src, size_t len) {
    void *page = (void*) ((uintptr_t) target & ~0xfff);
    if (mprotect(page, 4096, PROT_WRITE | PROT_EXEC)) {
        fprintf(stderr, "ERR: Cannot mprotect: %s\n", strerror(errno));
        exit(1);
    }

    memcpy(target, src, len);

    if (mprotect(page, 4096, PROT_EXEC)) {
        fprintf(stderr, "ERR: Cannot mprotect: %s\n", strerror(errno));
        exit(1);
    }
}

void hotpatch_prologue(void *target, void *addr) {
    hotpatch(target - sizeof addr, &addr, sizeof addr);
}

void hotpatch_code() {
    hotpatch_prologue(M0, M0_hdn);
    hotpatch_prologue(M1, M1_hdn);
    hotpatch_prologue(M2, M2_hdn);
    hotpatch_prologue(M3, M3_hdn);
    hotpatch_prologue(M4, M4_hdn);
}




int main() {
    hotpatch_code();

    load_table_address();
    asm("call *0x10(%rax)");

    load_table_address();
    asm("movq 0x10(%rax), %rax");
    asm("call *-0x08(%rax)");

    asm("call M0");

    return 0;
}
