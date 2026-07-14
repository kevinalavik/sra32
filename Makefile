.MAKEFLAGS: -s
.PHONY: all clean

all:
	@mkdir -p build
	@$(MAKE) -C emu all

clean:
	@$(MAKE) -C emu clean
	@rm -rf build
