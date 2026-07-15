.MAKEFLAGS: -s
.PHONY: all clean

all:
	@mkdir -p build
	@$(MAKE) -C emu all
	@$(MAKE) -C test all

clean:
	@$(MAKE) -C emu clean
	@$(MAKE) -C test clean
	@rm -rf build
