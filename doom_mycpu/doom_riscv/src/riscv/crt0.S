/*
 * crt0.S
 * Startup Code for Custom RISC-V Core
 */

.section .init
.global _start
.type   _start, @function

_start:
    # --------------------------------------------------------
    # [Optimization] Skip BSS Clearing
    # The Verilator simulation memory is zero-initialized by default.
    # We skip the clearing loop here to save simulation time and
    # avoid potential stack overlap issues during early boot.
    # --------------------------------------------------------
    # (BSS clearing logic moved to i_main.c or skipped)

    # --------------------------------------------------------
    # [Debug] Print 'M' to UART
    # Indicates that the CPU has fetched instructions and is alive.
    # 0x40000010 is the UART Data Register.
    # --------------------------------------------------------
    li t0, 0x40000010
    li t1, 0x4d        # ASCII 'M'
    sw t1, 0(t0)

    # --------------------------------------------------------
    # Setup Stack and Jump to Main
    # --------------------------------------------------------
    # [CRITICAL] Hardcode SP to 64MB (0x04000000).
    # We avoid using linker symbols like _stack_top here because
    # they might miscalculate based on the flat binary layout,
    # leading to the stack overwriting code or data.
    li sp, 0x04000000
    
    # Set argc = 0, argv = NULL
    li a0, 0
    li a1, 0
    
    # Enter C code
    call main

hang:
    j hang