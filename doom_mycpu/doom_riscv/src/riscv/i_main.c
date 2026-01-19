#include <stdio.h>
#include <stdlib.h>
#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"

// ============================================================================
// Linker Script Symbols
// These addresses are defined in picolibc.ld and mark the boundaries of memory sections.
// ============================================================================
extern unsigned long __data_source[];  // Flash address where initial values are stored
extern unsigned long __data_start[];   // RAM address where .data section begins
extern unsigned long __data_end[];     // RAM address where .data section ends
extern unsigned long __bss_start[];    // RAM address where .bss section begins
extern unsigned long __bss_end[];      // RAM address where .bss section ends

// ============================================================================
// Manual Memory Initialization
// Copies global variable values from Flash to RAM and clears uninitialized variables.
// ============================================================================
void System_Init_Memory(void) {
    unsigned long *src, *dst;

    printf("[SYS] System_Init_Memory: Copying .data...\n");
    printf("[SYS] Source: %p, Dest: %p, End: %p\n", __data_source, __data_start, __data_end);

    // 1. Copy .data section (Flash -> RAM)
    src = __data_source;
    dst = __data_start;
    while (dst < __data_end) {
        *dst++ = *src++;
    }

    printf("[SYS] System_Init_Memory: Clearing .bss...\n");
    
    // 2. Clear .bss section (Set to 0)
    dst = __bss_start;
    while (dst < __bss_end) {
        *dst++ = 0;
    }
    
    printf("[SYS] Memory initialized.\n");
}

int main(int argc, char** argv) 
{ 
    // [CRITICAL] Initialize memory before doing anything else.
    // Without this, global variables (like texture lists) will contain garbage data.
    System_Init_Memory();

    myargc = argc; 
    myargv = argv; 
 
    // Start the Doom Main Loop
    D_DoomMain (); 
 
    return 0; 
}