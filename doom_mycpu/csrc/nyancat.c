// 檔名：csrc/nyancat.c
#include <stdint.h>

// 定義 MMIO 基底位址 (根據你的 MemoryAccess.scala)
#define VGA_BASE        0x30000000

// 暫存器定義
#define VGA_CTRL        (*(volatile uint32_t*)(VGA_BASE + 0x04))
#define VGA_UPLOAD_ADDR (*(volatile uint32_t*)(VGA_BASE + 0x10))
#define VGA_STREAM_DATA (*(volatile uint32_t*)(VGA_BASE + 0x14))

// 調色盤基底 (根據你的新 VGA.scala，Palette 從 offset 0x400 開始)
// 範圍：0x30000400 ~ 0x300007FC
#define VGA_PALETTE(idx) (*(volatile uint32_t*)(VGA_BASE + 0x400 + ((idx) * 4)))

// 解析度常數 (Doom 原生解析度)
#define WIDTH  320
#define HEIGHT 200

// 簡單的延遲函式
void delay(int count) {
    for (volatile int i = 0; i < count; i++);
}

// 測試主程式
int main() {
    // 1. 初始化調色盤 (Palette Test)
    // 我們設定幾個顯眼的顏色來測試 8-bit index
    VGA_PALETTE(0) = 0x00; // Index 0: 黑 (R0 G0 B0)
    VGA_PALETTE(1) = 0x30; // Index 1: 紅 (R3 G0 B0) - bits [5:4]=3
    VGA_PALETTE(2) = 0x0C; // Index 2: 綠 (R0 G3 B0) - bits [3:2]=3
    VGA_PALETTE(3) = 0x03; // Index 3: 藍 (R0 G0 B3) - bits [1:0]=3
    VGA_PALETTE(4) = 0x3F; // Index 4: 白 (R3 G3 B3)
    
    // 2. 測試填滿背景 (Framebuffer Packing Test)
    // 我們用 Index 1 (紅色) 填滿整個螢幕
    // 因為現在一個 Word 包含 4 個像素 (8-bit * 4)，所以值是 0x01010101
    // 這樣寫入 32-bit word 會同時畫出 4 個紅色點
    VGA_UPLOAD_ADDR = 0; // 重置寫入位址
    uint32_t fill_color = 0x01010101; 
    
    // 總共有 320 * 200 = 64000 像素
    // 每次寫入 4 像素，所以跑 16000 次
    for (int i = 0; i < (WIDTH * HEIGHT) / 4; i++) {
        VGA_STREAM_DATA = fill_color;
    }
    
    // 3. 測試邊界 (Resolution Boundary Test)
    // 我們在四個角落畫白色 (Index 4)
    // 白色 packed word = 0x04040404
    uint32_t white_pixels = 0x04040404;

    // 左上 (0,0) -> Word 0
    VGA_UPLOAD_ADDR = 0;
    VGA_STREAM_DATA = white_pixels; 

    // 右上 (319, 0)
    // 該行最後一個 Word Index = (0 * 320 + 319) / 4 = 79
    VGA_UPLOAD_ADDR = 79; 
    VGA_STREAM_DATA = white_pixels; 

    // 左下 (0, 199)
    // Word Index = (199 * 320 + 0) / 4 = 15920
    VGA_UPLOAD_ADDR = 15920; 
    VGA_STREAM_DATA = white_pixels; 

    // 右下 (319, 199)
    // Word Index = (199 * 320 + 319) / 4 = 15999
    VGA_UPLOAD_ADDR = 15999; 
    VGA_STREAM_DATA = white_pixels;

    // 4. 啟用顯示
    // Bit 0 = Enable, Bit 1 = Blank (0=Show)
    // 寫入 1 (Enable=1, Blank=0)
    VGA_CTRL = 1; 

    // 死迴圈讓模擬器停在這裡，保持畫面
    while(1);
    
    return 0;
}