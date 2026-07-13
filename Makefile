.MAKEFLAGS: -s
.PHONY: all clean

all: emu test
	@mkdir -p build
	@$(MAKE) -C emu all
	@$(MAKE) -C test all

clean: emu test
	@$(MAKE) -C emu clean
	@$(MAKE) -C test clean
	@rm -rf build
