include ../conf.mk

TARGET := $(BUILD_DIR)/emu/sra32emu
SRC    := $(shell find src -type f -name '*.c')
OBJ    := $(patsubst src/%.c,$(BUILD_DIR)/emu/%.o,$(SRC))
DEP    := $(OBJ:.o=.d)

CFLAGS += -Iinclude

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "  LD $@"
	@$(CC) $(OBJ) -o $@

$(BUILD_DIR)/emu/%.o: src/%.c
	@echo "  CC $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR)/emu

-include $(DEP)
