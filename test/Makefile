include ../conf.mk

TEST_DIR := $(BUILD_DIR)/test

all: $(TEST_DIR)/test.bin

$(TEST_DIR)/test.bin: $(TEST_DIR)/test.elf
	sra32-elf-objcopy -O binary $< $@

$(TEST_DIR)/test.elf: $(TEST_DIR)/test.o test.ld
	sra32-elf-ld -T test.ld -o $@ $<

$(TEST_DIR)/test.o: test.s
	@mkdir -p $(TEST_DIR)
	sra32-elf-as $< -o $@

clean:
	rm -rf $(TEST_DIR)