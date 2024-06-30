CC = /usr/bin/gcc
CFLAGS = -g -Wall
LIBS = -lpanel -lmenu -lform -lncurses
SRC_DIR = ./src
SRC_TEST_DIR = ./testing
BUILD_DIR = ./build

# Source files in src/ directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files in the build directory
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: main

# Main target
main: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# File data unit testing
unit_testing: $(SRC_TEST_DIR)/unit_testing.c $(BUILD_DIR)/file_data.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Rule for compiling object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Explicit rule for compiling main.o without a corresponding .h file
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(SRC_DIR)/main.c -o $@

# Clean rule to remove build artifacts
clean:
	rm -rf $(BUILD_DIR) main unit_testing

.PHONY: all clean
