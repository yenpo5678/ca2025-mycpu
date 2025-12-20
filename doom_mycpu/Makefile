# MyCPU Root Makefile
# Refined to delegate simulation build to verilog/verilator/Makefile

include ../common/build.mk

.DEFAULT_GOAL := test

SIM_TIME ?= 1000000
SIM_VCD ?= trace.vcd
WRITE_VCD ?= 1

test:
	cd .. && sbt "project mmioTrap" test

# Refined: Run SBT, COPY the generated Top.v, then call sub-makefile
verilator:
	# 1. Generate Verilog using SBT in parent directory
	cd .. && PATH=$$HOME/.local/bin:$$PATH sbt "project mmioTrap" "runMain board.verilator.VerilogGenerator"
	
	# 2. Copy the generated Top.v to the local directory where Verilator expects it
	@echo "Copying generated Top.v to local directory..."
	cp ../2-mmio-trap/verilog/verilator/Top.v verilog/verilator/Top.v
	
	# 3. Delegate compilation to the sub-directory Makefile
	$(MAKE) -C verilog/verilator sim

# Alias for compatibility
verilator-sdl2: verilator

# Run simulation (Headless)
sim: verilator
	@if [ "$(WRITE_VCD)" = "0" ]; then \
		cd verilog/verilator/obj_dir && ./VTop -time $(SIM_TIME) $(subst src/main/resources/,../../../src/main/resources/,$(SIM_ARGS)); \
	else \
		cd verilog/verilator/obj_dir && ./VTop -vcd ../../../$(SIM_VCD) -time $(SIM_TIME) $(subst src/main/resources/,../../../src/main/resources/,$(SIM_ARGS)); \
	fi

# Run Demo (Nyancat with new hardware support)
demo: verilator
	@echo "Starting VGA demo (Custom Cartridge)..."
	@echo "   Display: 640x480 (SDL2 Window)"
	@echo "   Program: csrc/nyancat.asmbin (Updated for new hardware)"
	@echo "   Running for 500M cycles..."
	cd verilog/verilator/obj_dir && ./VTop -vga -instruction ../../../csrc/nyancat.asmbin -time 500000000
	@echo "Demo complete!"

indent:
	find . -name '*.scala' | xargs scalafmt
	clang-format -i verilog/verilator/*.cpp
	clang-format -i csrc/*.[ch]

compliance: check-riscof
	@echo "Running RISCOF compliance tests..."
	@cd ../tests && RISCOF_WORK=riscof_work_2mt ./run-compliance.sh 2-mmio-trap
	@rm -rf results && mkdir -p results
	@cp -r ../tests/riscof_work_2mt/* results/
	@echo "Compliance tests complete. See results/report.html"

clean:
	cd .. && sbt "project mmioTrap" clean
	rm -rf test_run_dir
	$(MAKE) -C verilog/verilator clean
	rm -f verilog/verilator/*.v verilog/verilator/*.fir verilog/verilator/*.anno.json
	rm -f $(SIM_VCD)

distclean: clean
	rm -rf results

.PHONY: verilator verilator-sdl2 test indent sim demo compliance clean distclean