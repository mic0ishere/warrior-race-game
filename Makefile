# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wformat-security
LDFLAGS = -lncurses -lm

# Target executable
TARGET = warrior_race

# Source files
SOURCES = main.c window.c level.c game.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = types.h window.h level.h game.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
