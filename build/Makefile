# =============================================================================
# Makefile — Digital Twin Forward Collision Warning System
# =============================================================================
# Targets:
#   make           — build with ncurses dashboard (default)
#   make plain     — build with plain printf dashboard (no ncurses needed)
#   make run       — build (ncurses) and run
#   make run-plain — build (plain) and run
#   make clean     — remove all build artefacts
#
# QNX cross-compilation:
#   Replace CC=gcc with CC=qcc and add -Vgcc_ntoaarch64le to CFLAGS.
# =============================================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -I./include
LDFLAGS  = -lm -lncurses

TARGET   = fcw_sim
SRCDIR   = src
BUILDDIR = build

SOURCES  = $(SRCDIR)/main.c                   \
           $(SRCDIR)/sensor_process.c         \
           $(SRCDIR)/digital_twin_process.c   \
           $(SRCDIR)/prediction_engine.c      \
           $(SRCDIR)/warning_system.c         \
           $(SRCDIR)/dash_proc.c

OBJECTS  = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

# ── Default: ncurses dashboard ────────────────────────────────────────────────
all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build OK (ncurses) -> $(BUILDDIR)/$(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Plain printf dashboard (no ncurses required) ──────────────────────────────
plain: $(BUILDDIR)
	$(CC) $(CFLAGS) -DPLAIN_DASH $(SOURCES) -o $(BUILDDIR)/$(TARGET) -lm
	@echo "Build OK (plain) -> $(BUILDDIR)/$(TARGET)"

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(BUILDDIR)/$(TARGET)

run-plain: plain
	./$(BUILDDIR)/$(TARGET)

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(TARGET)
	@echo "Cleaned."

.PHONY: all plain run run-plain clean
