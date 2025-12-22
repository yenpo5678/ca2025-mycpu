#include <stdlib.h>
#include <stdint.h>
#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"
#include "v_video.h"
#include "z_zone.h"
#include "m_argv.h"

#define MYCPU_VGA_BASE      0x20000000
#define VGA_CTRL_REG        (MYCPU_VGA_BASE + 0x04)
#define VGA_UPLOAD_ADDR     (MYCPU_VGA_BASE + 0x10)
#define VGA_STREAM_DATA     (MYCPU_VGA_BASE + 0x14)
#define VGA_PALETTE_BASE    (MYCPU_VGA_BASE + 0x400)

volatile uint32_t* const VGA_CTRL_PTR   = (uint32_t*)VGA_CTRL_REG;
volatile uint32_t* const VGA_ADDR_PTR   = (uint32_t*)VGA_UPLOAD_ADDR;
volatile uint32_t* const VGA_DATA_PTR   = (uint32_t*)VGA_STREAM_DATA;
volatile uint32_t* const VGA_PAL_PTR    = (uint32_t*)VGA_PALETTE_BASE;

#define MYCPU_INPUT_BASE    0x40000000 
volatile uint32_t* const INPUT_PTR      = (uint32_t*)MYCPU_INPUT_BASE;

// -----------------------------------------------------------------------------
// Memory Management - Fix Linker Error: I_ZoneBase, I_AllocLow
// -----------------------------------------------------------------------------
// DOOM requires a large chunk of memory (Zone Memory) for textures and sounds.
// We allocate a 4MB static array for DOOM usage.
// Note: Ensure the BSS/Data section in link.ld is large enough to hold this.
#define DOOM_HEAP_SIZE (4 * 1024 * 1024)
byte doom_heap[DOOM_HEAP_SIZE];

byte* I_ZoneBase (int* size)
{
    *size = DOOM_HEAP_SIZE;
    return doom_heap;
}

void* I_AllocLow (int length)
{
    // Use malloc for small memory allocations during system startup
    return malloc(length);
}

// -----------------------------------------------------------------------------
// System and Timing
// -----------------------------------------------------------------------------
unsigned long long get_cycles() {
    unsigned long long cycles;
    asm volatile ("rdcycle %0" : "=r" (cycles));
    return cycles;
}

#define CPU_FREQ 50000000  
#define TICKS_PER_SEC 35

int I_GetTime (void)
{
    unsigned long long cycles = get_cycles();
    return (int)((cycles * TICKS_PER_SEC) / CPU_FREQ);
}

void I_Init (void)
{
    *VGA_CTRL_PTR = 1; 
    *VGA_ADDR_PTR = 0;
}

void I_Quit (void) { while(1); }

void I_Error (char *error, ...) { while(1); }

// Fix Linker Error: I_WaitVBL
void I_WaitVBL(int count)
{
    // Simple delay or empty function for bare-metal.
    // Delay loop can be added here if the system runs too fast.
}

// -----------------------------------------------------------------------------
// Video Driver
// -----------------------------------------------------------------------------
void I_SetPalette (byte* palette)
{
    for (int i = 0; i < 256; i++)
    {
        byte r = gammatable[usegamma][*palette++];
        byte g = gammatable[usegamma][*palette++];
        byte b = gammatable[usegamma][*palette++];
        
        uint32_t r_2bit = (r >> 6) & 0x3;
        uint32_t g_2bit = (g >> 6) & 0x3;
        uint32_t b_2bit = (b >> 6) & 0x3;
        
        uint32_t color_entry = (r_2bit << 4) | (g_2bit << 2) | b_2bit;
        VGA_PAL_PTR[i] = color_entry;
    }
}

void I_FinishUpdate (void)
{
    *VGA_ADDR_PTR = 0x00000000;
    const uint32_t* src = (const uint32_t*)screens[0];
    int num_words = (SCREENWIDTH * SCREENHEIGHT) / 4;
    int i = 0;
    
    // Unrolled loop for performance
    for (; i < num_words - 8; i += 8)
    {
        *VGA_DATA_PTR = src[i];
        *VGA_DATA_PTR = src[i+1];
        *VGA_DATA_PTR = src[i+2];
        *VGA_DATA_PTR = src[i+3];
        *VGA_DATA_PTR = src[i+4];
        *VGA_DATA_PTR = src[i+5];
        *VGA_DATA_PTR = src[i+6];
        *VGA_DATA_PTR = src[i+7];
    }
    for (; i < num_words; i++) {
        *VGA_DATA_PTR = src[i];
    }
}

void I_InitGraphics (void)
{
    I_Init();
    // Must allocate memory for the Framebuffer
    screens[0] = (byte *)malloc (SCREENWIDTH * SCREENHEIGHT);
}

// Fix Linker Error: I_MarkDirtyLines (Important: DOOM relies on this for partial updates)
// Since we perform a full screen update (FinishUpdate), this can remain empty.
void I_MarkDirtyLines (int top, int bottom) {}

void I_ReadScreen (byte* scr) {}
void I_UpdateNoBlit (void) {}
void I_ShutdownGraphics (void) {}
void I_StartFrame (void) {}

// -----------------------------------------------------------------------------
// Input Driver (Stub)
// -----------------------------------------------------------------------------
void I_StartTic (void) {}

// Fix Linker Error: I_SetRelativeMode (Mouse related)
void I_SetRelativeMode(int grab) {}

// Fix Linker Error: I_BaseTiccmd (Network/Input related)
void I_BaseTiccmd(ticcmd_t* cmd) {}

// Fix Linker Error: I_Tactile (Force feedback related)
void I_Tactile(int on, int off, int total) {}

// -----------------------------------------------------------------------------
// Sound (Stubs) - Fix parameters to match header definition
// -----------------------------------------------------------------------------
void I_InitSound() {}
void I_SubmitSound() {}
void I_ShutdownSound() {}
void I_SetChannels() {}

// Fix: I_StartSound parameters and return value
int I_StartSound(int id, int vol, int sep, int pitch, int priority) { return 0; }

// Fix: I_StopSong changed to void parameter based on error message
void I_StopSong(void) {} 

// Fix: I_PlaySong
void I_PlaySong(void *data, int looping) {}

// Fix Linker Error: I_SetSfxVolume
void I_SetSfxVolume(int volume) {}

// Fix Linker Error: I_SetMusicVolume
void I_SetMusicVolume(int volume) {}

void I_PauseSong(int handle) {}
void I_ResumeSong(int handle) {}
void I_UnRegisterSong(int handle) {}
int  I_RegisterSong(void *data) { return 1; }
int  I_GetSfxLumpNum(void* sfx) { return 0; }
void I_StopSound(int handle) {}
int  I_SoundIsPlaying(int handle) { return 0; }
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}

// -----------------------------------------------------------------------------
// Network (Stubs)
// -----------------------------------------------------------------------------
void I_InitNetwork (void) { doomcom = malloc (sizeof (*doomcom)); }
void I_NetCmd (void) {}