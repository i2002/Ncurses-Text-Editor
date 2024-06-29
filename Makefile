CC = /usr/bin/gcc
CFLAGS = -g -Wall
LIBS = -lpanel -lmenu -lform -lncurses
BUILD = ./build

# Source files
SRCS = file_data.c file_view.c dialogs.c text_editor.c

# Object files in the build directory
OBJS = $(SRCS:%.c=$(BUILD)/%.o)

# Default target
all: main

# Main target
main: $(OBJS)
	$(CC) -o $@ main.c $^ $(LIBS)

# Rule for compiling object files
$(BUILD)/%.o: %.c %.h
	@mkdir -p $(BUILD)
	$(CC) -c $(CFLAGS) $< -o $@

# Clean rule to remove build artifacts
clean:
	rm -rf $(BUILD) main

.PHONY: all clean
