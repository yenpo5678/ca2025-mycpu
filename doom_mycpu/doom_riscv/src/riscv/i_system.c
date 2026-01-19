#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"
#include "d_net.h"     
#include "g_game.h"
#include "i_system.h"

// Import mini-printf for lightweight formatting
extern int mini_vsnprintf(char* buffer, unsigned int buffer_len, const char *fmt, va_list va);

// =============================================================================
// Memory Management (Simple Linear Allocator)
// =============================================================================

// Define 8MB Static Heap in .bss section
#define BIG_HEAP_SIZE (8 * 1024 * 1024) 
static uint64_t my_static_heap[BIG_HEAP_SIZE / 8]; // uint64 ensures 8-byte alignment
static int heap_allocated_bytes = 0;

// Simple Bump Pointer Allocator (Alloc only, No Free)
void* My_Simple_Malloc(int size) {
    // Force 8-byte alignment
    if (size % 8 != 0) size += (8 - (size % 8));
    
    if (heap_allocated_bytes + size > BIG_HEAP_SIZE) {
        printf("[SYS] My_Malloc: OUT OF MEMORY! Req %d, Left %d\n", 
               size, BIG_HEAP_SIZE - heap_allocated_bytes);
        while(1); // Halt on OOM
    }
    
    uint8_t* base_ptr = (uint8_t*)my_static_heap;
    void* ptr = (void*)(base_ptr + heap_allocated_bytes);
    heap_allocated_bytes += size;
    return ptr;
}

// Main Zone Memory Allocation for Doom
byte* I_ZoneBase (int* size){
    *size = 4 * 1024 * 1024; // Allocate 4MB for Zone Memory
    printf("[SYS] I_ZoneBase: Allocating %d bytes...\n", *size);
    return (byte*)My_Simple_Malloc(*size);
}

// Low-level allocation (Cache, Screen Buffer, etc.)
byte* I_AllocLow (int length){
    printf("[SYS] I_AllocLow: Asking My_Malloc for %d bytes...\n", length);
    byte* mem = (byte*)My_Simple_Malloc(length);
    printf("[SYS] I_AllocLow: Got %p. Clearing...\n", mem);
    memset(mem, 0, length);
    printf("[SYS] I_AllocLow: Done.\n");
    return mem;
}

// =============================================================================
// Network Stubs (Fixes "Doomcom buffer invalid")
// =============================================================================

void I_InitNetwork (void) {
    printf("[SYS] I_InitNetwork: Setting up dummy network...\n");
    
    // Allocate and initialize doomcom struct
    doomcom = (doomcom_t*) My_Simple_Malloc(sizeof(doomcom_t));
    memset(doomcom, 0, sizeof(doomcom_t));

    // Populate with dummy data to bypass D_CheckNetGame
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = 1;
    doomcom->numnodes = 1;
    doomcom->deathmatch = 0;
    doomcom->consoleplayer = 0;
    doomcom->ticdup = 1;
    doomcom->extratics = 0;
}

void I_NetCmd (void) {
    // No networking required for single player
}

// =============================================================================
// Time & System (Fixes Frozen Screen / Input Lag)
// =============================================================================

// [CRITICAL FIX] Turbo Timer
// Forces the game engine to advance a frame every time time is checked.
int I_GetTime (void) {
    static int fake_timer = 0;
    fake_timer++; 
    return fake_timer;
}

// Disable VSync waiting (Simulated environment has no VBlank)
void I_WaitVBL(int count) {}

// System Initialization
void I_Init (void) { 
    I_InitSound(); 
    I_InitNetwork(); // Must be called to prevent network check failure
}

void I_Quit (void) { 
    printf("[SYS] I_Quit called.\n");
    while(1); 
}

// Error Handling (Formatted Output)
void I_Error (char *error, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, error);
    mini_vsnprintf(buffer, sizeof(buffer), error, va);
    va_end(va);

    printf("\n[SYS] I_Error: %s\n", buffer);
    while(1); // Halt execution
}

// =============================================================================
// Stubs (Empty functions to satisfy Linker)
// =============================================================================

void I_StartTic (void) {}
void I_SetRelativeMode(boolean enabled) {}
ticcmd_t* I_BaseTiccmd(void) { static ticcmd_t emptycmd; return &emptycmd; }
void I_Tactile(int on, int off, int total) {}
void I_StdOutPacketError(void) {}
void I_AddExitFunc(void (*func)()) {}
void I_BeginRead(void) {}
void I_EndRead(void) {}