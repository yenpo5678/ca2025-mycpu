CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size

# 引入原始碼列表
include ../sources.mk

# 強制過濾掉重複的檔案
SOURCES_doom := $(filter-out d_main.c s_sound.c i_net.c w_wad.c,$(SOURCES_doom))

# 這些是我們自己寫的裸機版本檔案
SOURCES_doom_arch := \
    d_main.c \
    i_main.c \
    i_sound.c \
    i_system.c \
    i_video.c \
    w_wad.c \
    s_sound.c \
    console.c \
    libc_backend.c \
    crt0.S \
    mini-printf.c

# Linker 設定
LDFLAGS=-Tpicolibc.ld \
        -Wl,--defsym=__flash=0x00001000 \
        -Wl,--defsym=__flash_size=0x01000000 \
        -Wl,--defsym=__ram=0x01000000 \
        -Wl,--defsym=__ram_size=0x01000000 \
        -Wl,--defsym=__stack_size=0x4000 \
        -Wl,-Bstatic,--strip-debug,--gc-sections \
        -lc -lm -lgcc \
        -nostartfiles

# 編譯參數 (強制使用軟體乘除法 rv32i)
CFLAGS=-Wall -O2 -march=rv32i -mabi=ilp32 -mcmodel=medany \
       --specs=picolibc.specs \
       -ffreestanding -flto  -mno-save-restore -fomit-frame-pointer \
       -ffunction-sections -fdata-sections \
       -I..

CFLAGS += -DSCREENWIDTH=320 -DSCREENHEIGHT=200

VPATH = .:..

all: doom-riscv.elf

# [關鍵] 這裡加入了 doom_wad.o 的相依性
doom-riscv.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch) doom_wad.o
	$(CC) $(CFLAGS) $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch) doom_wad.o $(LDFLAGS) -o $@
	$(SIZE) $@

# [新規則] 自動把 WAD 檔轉成 .o 檔
doom_wad.o: doom1.wad
	$(OBJCOPY) -I binary -O elf32-littleriscv -B riscv --rename-section .data=.rodata,alloc,load,readonly,data,contents doom1.wad doom_wad.o

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

.PHONY: all clean
.PRECIOUS: *.elf