include ../conf.mk

TARGET := $(BUILD_DIR)/firmware/sra32ka-firm-rev1

C_SRC := $(shell find . -type f -name '*.c')
S_SRC := $(shell find . -type f -name '*.S')

OBJ := \
	$(patsubst %.c,$(BUILD_DIR)/firmware/%.o,$(C_SRC)) \
	$(patsubst %.S,$(BUILD_DIR)/firmware/%.o,$(S_SRC))

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR)/firmware:
	@echo -e "\tMKDIR $@"
	@mkdir -p $@

$(TARGET): $(OBJ)
	@echo -e "\tLD $@"
	@$(SRA32_CC) $(OBJ) -o $@ $(SRA32_CFLAGS) $(SRA32_LDFLAGS)

$(BUILD_DIR)/firmware/%.o: %.c | $(BUILD_DIR)/firmware
	@echo -e "\tCC $<"
	@mkdir -p $(dir $@)
	@$(SRA32_CC) $(SRA32_CFLAGS) -c $< -o $@

$(BUILD_DIR)/firmware/%.o: %.S | $(BUILD_DIR)/firmware
	@echo -e "\tAS $<"
	@mkdir -p $(dir $@)
	@$(SRA32_CC) $(SRA32_CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR)/firmware