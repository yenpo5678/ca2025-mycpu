#include <stdlib.h>
#include <stdint.h>       // Added for uint32_t definition
#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "i_system.h"     // Added for I_AllocLow
#include "v_video.h"
#include "i_video.h"

// ============================================================================
// Hardware Definitions (MMIO Base Addresses)
// Note: These must match your Verilator/FPGA Address Map.
// ============================================================================
#define MYCPU_VGA_BASE      0x30000000
#define VGA_CTRL_REG        (MYCPU_VGA_BASE + 0x04)
#define VGA_UPLOAD_ADDR     (MYCPU_VGA_BASE + 0x10)
#define VGA_STREAM_DATA     (MYCPU_VGA_BASE + 0x14)
#define VGA_PALETTE_BASE    (MYCPU_VGA_BASE + 0x400)

// Volatile pointers prevent compiler optimization from removing MMIO writes
volatile uint32_t* const VGA_CTRL_PTR  = (uint32_t*)VGA_CTRL_REG;
volatile uint32_t* const VGA_ADDR_PTR  = (uint32_t*)VGA_UPLOAD_ADDR;
volatile uint32_t* const VGA_DATA_PTR  = (uint32_t*)VGA_STREAM_DATA;
volatile uint32_t* const VGA_PAL_PTR   = (uint32_t*)VGA_PALETTE_BASE;

// ============================================================================
// Palette Management
// Converts Doom's 8-bit RGB (0-255) to the Hardware's 2-bit RGB (0-3)
// ============================================================================
void I_SetPalette (byte* palette)
{
    for (int i = 0; i < 256; i++)
    {
        // Apply Gamma correction
        byte r = gammatable[usegamma][*palette++];
        byte g = gammatable[usegamma][*palette++];
        byte b = gammatable[usegamma][*palette++];

        // Downsample to 2 bits per channel (6-bit color depth)
        // Adjust this bit-shift if your VGA hardware supports more colors
        uint32_t r_2bit = (r >> 6) & 0x3;
        uint32_t g_2bit = (g >> 6) & 0x3;
        uint32_t b_2bit = (b >> 6) & 0x3;

        // Pack into a single word and write to hardware palette
        uint32_t color_entry = (r_2bit << 4) | (g_2bit << 2) | b_2bit;
        VGA_PAL_PTR[i] = color_entry;
    }
}

// ============================================================================
// Frame Update (Software Blitter)
// Copies the internal framebuffer to the VGA hardware memory
// ============================================================================
void I_FinishUpdate (void)
{
    // 1. Reset Hardware Address Pointer to 0 (Top-Left pixel)
    *VGA_ADDR_PTR = 0x00000000;

    // 2. Prepare Source Pointer (Doom's internal screen)
    const uint32_t* src = (const uint32_t*)screens[0];
    
    // We process 4 pixels at a time (uint32_t), so divide by 4
    int num_words = (SCREENWIDTH * SCREENHEIGHT) / 4;
    int i = 0;

    // 3. Loop Unrolling for Performance
    // Reduces branch instruction overhead, critical for slower soft-cores
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
    // Handle remaining words
    for (; i < num_words; i++) {
        *VGA_DATA_PTR = src[i];
    }
}

// ============================================================================
// Initialization & Stubs
// ============================================================================
void I_InitGraphics (void)
{
    // Enable VGA Hardware
    *VGA_CTRL_PTR = 1;
    *VGA_ADDR_PTR = 0;
    
    // Allocate Framebuffer using our custom allocator from i_system.c
    // NOTE: Changed from 'malloc' to 'I_AllocLow' for bare-metal safety
    screens[0] = (byte *)I_AllocLow (SCREENWIDTH * SCREENHEIGHT);
}

void I_ShutdownGraphics (void) {}
void I_UpdateNoBlit (void) {}
void I_StartFrame (void) {}
void I_ReadScreen (byte* scr) {}
void I_MarkDirtyLines (int top, int bottom) {}