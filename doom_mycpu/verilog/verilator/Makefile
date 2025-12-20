# ==========================================
#  verilog/verilator/Makefile
#  負責編譯 Verilator 模擬器並自動連結 SDL2
# ==========================================

# 1. 自動抓取系統的 SDL2 設定
SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

# 2. 定義來源檔案
# Top.v 是由 Chisel 產生的，sim.cpp 是我們寫的
SRCS = sim.cpp Top.v ../../src/main/resources/vsrc/TrueDualPortRAM32.v

# 3. 定義 Verilator 參數
# -DENABLE_SDL2: 開啟 C++ 裡的畫圖開關
# -CFLAGS / -LDFLAGS: 告訴編譯器 SDL2 在哪裡
VERILATOR_FLAGS = --trace --exe --cc \
                  -Wno-WIDTHEXPAND -Wno-WIDTH \
                  -CFLAGS "-DENABLE_SDL2 $(SDL_CFLAGS)" \
                  -LDFLAGS "$(SDL_LDFLAGS)"

.PHONY: all sim clean

all: sim

sim:
	@echo "--- [Sub-Makefile] Building Verilator Simulation ---"
	# 產生 C++ 碼 (obj_dir/)
	verilator $(VERILATOR_FLAGS) $(SRCS)
	# 編譯 C++ 碼產生執行檔
	$(MAKE) -C obj_dir -f VTop.mk
	@echo "--- [Sub-Makefile] Build Success! ---"

clean:
	rm -rf obj_dir
	rm -f sim