include ../conf.mk

TARGET := $(BUILD_DIR)/emu/sra32emu
SRC    := $(wildcard *.c)
OBJ    := $(patsubst %.c,$(BUILD_DIR)/emu/%.o,$(SRC))

.PHONY: all clean
all: $(TARGET)

$(BUILD_DIR)/emu:
	@echo -e "\tMKDIR $@"
	@mkdir -p $@

$(TARGET): $(OBJ)
	@echo -e "\tLD $@"
	@$(CC) $(OBJ) -o $@ $(CFLAGS)

$(BUILD_DIR)/emu/%.o: %.c sra32.h | $(BUILD_DIR)/emu
	@echo -e "\tCC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR)/emu