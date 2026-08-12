
CC ?= gcc
CFLAGS = -O2 -fopenmp -Wall -Wextra -Iinclude

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
CORE_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
DEPS = include/ecosystem.h $(SRC_DIR)/internal.h
TARGET = ecosystem
TEST_TARGET = test_runner

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.c $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(CORE_OBJS) $(BUILD_DIR)/tests/test_simulation.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

