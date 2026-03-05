# =============================================================================
# Makefile — Digital Twin Forward Collision Warning System
# =============================================================================
# Usage:
#   make          — build the fcw_sim binary
#   make run      — build and run the simulation
#   make clean    — remove all build artefacts
#
# QNX cross-compilation note:
#   Replace CC with qcc and add -Vgcc_ntoaarch64le for Raspberry Pi 4 target.
#   All source files and logic are identical on QNX — only the compiler changes.
# =============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -I./include
LDFLAGS = -lm

TARGET  = fcw_sim
SRCDIR  = src
BUILDDIR= build

# All source files in src/
SOURCES = $(SRCDIR)/main.c              \
          $(SRCDIR)/sensor_process.c    \
          $(SRCDIR)/digital_twin_process.c \
          $(SRCDIR)/prediction_engine.c \
          $(SRCDIR)/warning_system.c

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

# ── Default target ────────────────────────────────────────────────────────────
all: $(BUILDDIR)/$(TARGET)

# ── Link ──────────────────────────────────────────────────────────────────────
$(BUILDDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo ""
	@echo "Build successful → $(BUILDDIR)/$(TARGET)"

# ── Compile each .c to .o ─────────────────────────────────────────────────────
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Create build directory if missing ─────────────────────────────────────────
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# ── Run the simulation ────────────────────────────────────────────────────────
run: all
	@echo ""
	./$(BUILDDIR)/$(TARGET)

# ── Remove build artefacts ────────────────────────────────────────────────────
clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(TARGET)
	@echo "Cleaned build directory."

.PHONY: all run clean
