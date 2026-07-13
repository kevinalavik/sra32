include ../conf.mk

SRC_C    := $(wildcard *.c)
SRC_S    := $(wildcard *.S)
TARGETS  := $(patsubst %.c,$(BUILD_DIR)/test/%,$(SRC_C)) \
            $(patsubst %.S,$(BUILD_DIR)/test/%,$(SRC_S))

.PHONY: all clean

all: $(TARGETS)

$(BUILD_DIR)/test:
	@echo -e "\tMKDIR $@"
	@mkdir -p $@

$(BUILD_DIR)/test/%: %.c | $(BUILD_DIR)/test
	@echo -e "\tCC $<"
	@$(SRA32_CC) $(SRA32_CFLAGS) $< -o $@ $(SRA32_LDFLAGS)

$(BUILD_DIR)/test/%: %.S | $(BUILD_DIR)/test
	@echo -e "\tAS $<"
	@$(SRA32_CC) $(SRA32_CFLAGS) $< -o $@ $(SRA32_LDFLAGS)