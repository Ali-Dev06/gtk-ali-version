# Makefile for Aquarium GTK Application
# Compiles the aquarium application with proper GTK3 and json-c dependencies

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -O2
# --export-dynamic lets the dynamic "Custom C Function" button find functions by name at runtime.
LDFLAGS = -pthread -lm -Wl,--export-dynamic

# GTK libraries (json-c was unused and has been dropped).
# gmodule-export-2.0 is required by button_manager.c (runtime function lookup).
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 gmodule-export-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0 gmodule-export-2.0)
JSON_LIBS =

# Source files
SOURCES = main.c aquarium.c structures.c movement.c boids.c ecosystem.c food.c particles.c fish_catalog.c button_manager.c
HEADERS = aquarium.h structures.h fish_catalog.h button_manager.h
OBJECTS = $(SOURCES:.c=.o)
TARGET = aquarium

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(GTK_CFLAGS) $(GTK_LIBS) $(JSON_LIBS) $(LDFLAGS)

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build the aquarium executable (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  rebuild - Clean and build"
	@echo "  help    - Show this help message"

.PHONY: all clean rebuild help
