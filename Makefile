.MAKEFLAGS: -s
.PHONY: all clean

all: emu test firmware
	@mkdir -p build
	@$(MAKE) -C emu all
	@$(MAKE) -C test all
	@$(MAKE) -C firmware all

clean: emu test firmware
	@$(MAKE) -C emu clean
	@$(MAKE) -C test clean
	@$(MAKE) -C firmware clean
	@rm -rf build
