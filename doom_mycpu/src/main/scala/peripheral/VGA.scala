// SPDX-License-Identifier: MIT

package peripheral

import chisel3._
import chisel3.util._
import riscv.Parameters

/**
 * VGA peripheral for Doom: 320x200 resolution, 8-bit Color (256 Palette)
 *
 * Memory map (Base: 0x30000000):
 * 0x000: ID          - (RO) 0x56474132 ('VGA2')
 * 0x004: CTRL        - Display enable, blank, swap, frame select, int enable
 * 0x008: STATUS      - Vblank, safe to swap, etc.
 * 0x00C: INTR_STATUS - W1C
 * 0x010: UPLOAD_ADDR - Framebuffer upload address
 * 0x014: STREAM_DATA - 4 pixels packed in 32-bit word (8-bit per pixel)
 * 0x400-0x7FC: PALETTE - 256 x 6-bit colors (Start at offset 0x400)
 *
 * Video Configuration:
 * - Logical Resolution: 320x200 (Doom Native)
 * - Physical Output:    640x480 @ 72Hz (Standard VGA)
 * - Scaling:            2x Integer Scaling (320->640, 200->400)
 * - Margins:            Top/Bottom 40 lines black border
 * - Color Depth:        8-bit Index -> 6-bit Output (R2G2B2) via Palette RAM
 */
class VGA extends Module {
  val io = IO(new Bundle {
    val bundle      = new RAMBundle      // MMIO interface (CPU clock domain)
    val pixClock    = Input(Clock())     // VGA pixel clock (31.5 MHz)
    val hsync       = Output(Bool())
    val vsync       = Output(Bool())
    val rrggbb      = Output(UInt(6.W))  // 6-bit color output (R2G2B2)
    val activevideo = Output(Bool())
    val intr        = Output(Bool())
    val x_pos       = Output(UInt(10.W))
    val y_pos       = Output(UInt(10.W))
  })

  // ============ VGA Timing Parameters (640x480 @ 72Hz) ============
  val H_ACTIVE = 640
  val H_FP     = 24
  val H_SYNC   = 40
  val H_BP     = 128
  val H_TOTAL  = H_ACTIVE + H_FP + H_SYNC + H_BP // 832

  val V_ACTIVE = 480
  val V_FP     = 9
  val V_SYNC   = 3
  val V_BP     = 28
  val V_TOTAL  = V_ACTIVE + V_FP + V_SYNC + V_BP // 520

  // ============ Doom Resolution Scaling ============
  val FRAME_WIDTH    = 320
  val FRAME_HEIGHT   = 200
  val SCALE_FACTOR   = 2                  // 2x scaling
  val DISPLAY_WIDTH  = FRAME_WIDTH * SCALE_FACTOR  // 640 (Full Width)
  val DISPLAY_HEIGHT = FRAME_HEIGHT * SCALE_FACTOR // 400
  val LEFT_MARGIN    = (H_ACTIVE - DISPLAY_WIDTH) / 2  // 0
  val TOP_MARGIN     = (V_ACTIVE - DISPLAY_HEIGHT) / 2 // 40 lines top margin

  // ============ Framebuffer Size ============
  // Note: 320*200*1 byte = 64,000 bytes (~62.5KB)
  // Ensure your FPGA has enough BRAM! 
  // If size is an issue, we strictly use 1 frame (Double buffering removed for space)
  val NUM_FRAMES       = 1 
  val PIXELS_PER_FRAME = FRAME_WIDTH * FRAME_HEIGHT // 64000
  val WORDS_PER_FRAME  = PIXELS_PER_FRAME / 4       // 16000 words (4 pixels per 32-bit word)
  val TOTAL_WORDS      = NUM_FRAMES * WORDS_PER_FRAME
  val ADDR_WIDTH       = log2Ceil(TOTAL_WORDS)      // ~14 bits

  // ============ Memory Modules ============
  // Framebuffer: True Dual Port RAM
  val framebuffer = Module(new TrueDualPortRAM32(TOTAL_WORDS, ADDR_WIDTH))
  
  // Palette RAM: 256 entries x 6-bit color
  // We use SyncReadMem to infer a BRAM/LUTRAM with dual ports (CPU Write, VGA Read)
  val paletteRAM = SyncReadMem(256, UInt(6.W))

  // ============ CPU Clock Domain (sysclk) ============
  val sysClk = clock

  // Registers
  val ctrlReg       = RegInit(0.U(32.W))
  val intrStatusReg = RegInit(0.U(32.W))
  val uploadAddrReg = RegInit(0.U(32.W))

  val ctrl_en        = ctrlReg(0)
  val ctrl_blank     = ctrlReg(1)
  val ctrl_vblank_ie = ctrlReg(8)

  // Cross-clock wires
  val wire_in_vblank = Wire(Bool())

  withClock(sysClk) {
    // MMIO Decoding - Expanded to 0xFFF to cover Palette at 0x400
    val addr             = io.bundle.address & 0xfff.U 
    val addr_id          = addr === 0x00.U
    val addr_ctrl        = addr === 0x04.U
    val addr_status      = addr === 0x08.U
    val addr_intr_status = addr === 0x0c.U
    val addr_upload_addr = addr === 0x10.U
    val addr_stream_data = addr === 0x14.U
    
    // Palette at 0x400 - 0x7FC (1024 bytes space for 256 colors)
    val addr_palette     = (addr >= 0x400.U) && (addr < 0x800.U)
    val palette_idx      = (addr - 0x400.U) >> 2

    // CDC Status
    val vblank_synced    = RegNext(RegNext(wire_in_vblank))
    
    // Interrupt Generation
    val vblank_prev        = RegNext(vblank_synced)
    val vblank_rising_edge = vblank_synced && !vblank_prev
    when(vblank_rising_edge && ctrl_vblank_ie) {
      intrStatusReg := 1.U
    }
    io.intr := (intrStatusReg =/= 0.U) && ctrl_vblank_ie

    // MMIO Read
    io.bundle.read_data := MuxLookup(addr, 0.U)(
      Seq(
        0x00.U -> 0x56474132.U, // 'VGA2'
        0x04.U -> ctrlReg,
        0x08.U -> Cat(0.U(30.W), true.B, vblank_synced), // simplified status
        0x0c.U -> intrStatusReg,
        0x10.U -> uploadAddrReg
      )
    )

    // MMIO Write & Framebuffer Access
    val fb_write_en   = WireDefault(false.B)
    val fb_write_addr = WireDefault(0.U(ADDR_WIDTH.W))
    val fb_write_data = WireDefault(0.U(32.W))

    when(io.bundle.write_enable) {
      when(addr_ctrl) {
        ctrlReg := io.bundle.write_data
      }.elsewhen(addr_intr_status) {
        intrStatusReg := intrStatusReg & ~io.bundle.write_data
      }.elsewhen(addr_upload_addr) {
        uploadAddrReg := io.bundle.write_data
      }.elsewhen(addr_stream_data) {
        // Stream 4 pixels (8-bit each)
        val word_idx = uploadAddrReg(ADDR_WIDTH-1, 0)
        fb_write_en   := true.B
        fb_write_addr := word_idx
        fb_write_data := io.bundle.write_data
        
        // Auto-increment (wrap at frame end)
        val next_addr = word_idx + 1.U
        uploadAddrReg := Mux(next_addr >= WORDS_PER_FRAME.U, 0.U, next_addr)
      }.elsewhen(addr_palette) {
        // Write to Palette RAM
        paletteRAM.write(palette_idx, io.bundle.write_data(5, 0))
      }
    }

    framebuffer.io.clka  := clock
    framebuffer.io.wea   := fb_write_en
    framebuffer.io.addra := fb_write_addr
    framebuffer.io.dina  := fb_write_data
  }

  // ============ Pixel Clock Domain (pixclk) ============
  withClock(io.pixClock) {
    val h_count = RegInit(0.U(10.W))
    val v_count = RegInit(0.U(10.W))

    // Counters
    when(h_count === (H_TOTAL - 1).U) {
      h_count := 0.U
      when(v_count === (V_TOTAL - 1).U) { v_count := 0.U }
      .otherwise { v_count := v_count + 1.U }
    }.otherwise { h_count := h_count + 1.U }

    // Sync Generation (Delay matched to pipeline depth: 3 cycles)
    // Pipeline: AddrGen(0) -> FB_Read(1) -> Palette_Read(2) -> Out(3)
    val h_sync_pulse = (h_count >= (H_ACTIVE + H_FP).U) && (h_count < (H_ACTIVE + H_FP + H_SYNC).U)
    val v_sync_pulse = (v_count >= (V_ACTIVE + V_FP).U) && (v_count < (V_ACTIVE + V_FP + V_SYNC).U)
    
    // Delay chain for sync signals (3 cycles)
    val hsync_d1 = RegNext(!h_sync_pulse)
    val vsync_d1 = RegNext(!v_sync_pulse)
    val hsync_d2 = RegNext(hsync_d1)
    val vsync_d2 = RegNext(vsync_d1)
    io.hsync := RegNext(hsync_d2) // d3
    io.vsync := RegNext(vsync_d2) // d3

    // Pixel Coordinate Logic
    val h_active = h_count < H_ACTIVE.U
    val v_active = v_count < V_ACTIVE.U

    val x_px = RegNext(h_count) // Cycle 1
    val y_px = RegNext(v_count) // Cycle 1
    
    // Scaling Logic (2x) - Simple shift
    // x range [0, 639] -> [0, 319]
    // y range [40, 439] -> [0, 199]
    val rel_y_raw = y_px - TOP_MARGIN.U
    val in_margin_y = (y_px < TOP_MARGIN.U) || (y_px >= (TOP_MARGIN + DISPLAY_HEIGHT).U)
    
    val frame_x = x_px >> 1
    val frame_y = rel_y_raw >> 1
    
    // Address Calculation
    val pixel_addr = frame_y * FRAME_WIDTH.U + frame_x
    val word_addr  = pixel_addr >> 2 // Divide by 4 (4 pixels per word)
    val byte_sel   = pixel_addr(1, 0) // 0,1,2,3

    // --- PIPELINE STAGE 1: Framebuffer Read ---
    framebuffer.io.clkb  := io.pixClock
    // Ensure we don't read out of bounds during margins (can cause garbage)
    framebuffer.io.addrb := word_addr 
    
    // Save control signals for next stages
    val in_margin_y_d1 = RegNext(in_margin_y)
    val h_active_d1    = RegNext(h_active)
    val v_active_d1    = RegNext(v_active)
    val byte_sel_d1    = RegNext(byte_sel)
    
    // --- PIPELINE STAGE 2: Data Extract & Palette Read ---
    // fb_word available here
    val fb_word = framebuffer.io.doutb
    
    // Extract 8-bit pixel index
    // Note: Assuming Little Endian packing
    val pixel_idx = MuxLookup(byte_sel_d1, 0.U)(
      Seq(
        0.U -> fb_word(7, 0),
        1.U -> fb_word(15, 8),
        2.U -> fb_word(23, 16),
        3.U -> fb_word(31, 24)
      )
    )
    
    // Read Palette RAM (Index -> Color)
    // SyncReadMem read takes 1 cycle
    val palette_color_out = paletteRAM.read(pixel_idx)
    
    // Save control signals
    val in_margin_y_d2 = RegNext(in_margin_y_d1)
    val h_active_d2    = RegNext(h_active_d1)
    val v_active_d2    = RegNext(v_active_d1)

    // --- PIPELINE STAGE 3: Output ---
    // palette_color_out available here
    
    val ctrl_blank_sync = RegNext(RegNext(ctrl_blank))
    val ctrl_en_sync    = RegNext(RegNext(ctrl_en))

    val is_black = ctrl_blank_sync || !ctrl_en_sync || in_margin_y_d2
    
    io.rrggbb := Mux(h_active_d2 && v_active_d2 && !is_black, palette_color_out, 0.U)
    io.activevideo := h_active_d2 && v_active_d2
    
    // VBlank Logic
    val in_vblank = v_count >= V_ACTIVE.U
    wire_in_vblank := in_vblank
    
    // Debug Position (aligned to d3)
    io.x_pos := RegNext(RegNext(x_px))
    io.y_pos := RegNext(RegNext(y_px))
  }
}