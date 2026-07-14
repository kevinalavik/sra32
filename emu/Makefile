include ../conf.mk

TARGET := $(BUILD_DIR)/emu/sra32emu
SRC    := $(shell find . -type f -name '*.c')
OBJ    := $(patsubst %.c,$(BUILD_DIR)/emu/%.o,$(SRC))

.PHONY: all clean
all: $(TARGET)

$(BUILD_DIR)/emu:
	@echo "  MKDIR $@"
	@mkdir -p $@

$(TARGET): $(OBJ)
	@echo "  LD $@"
	@$(CC) $(OBJ) -o $@ $(CFLAGS)

$(BUILD_DIR)/emu/%.o: %.c sra32.h | $(BUILD_DIR)/emu
	@echo "  CC $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR)/emu