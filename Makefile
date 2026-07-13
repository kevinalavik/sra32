.MAKEFLAGS: -s
.PHONY: all clean

all: emu
	@mkdir -p build
	@$(MAKE) -C emu all

clean: emu
	@$(MAKE) -C emu clean
	@rm -rf build
