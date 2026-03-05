# =============================================================================
# Makefile — Digital Twin Forward Collision Warning System
# =============================================================================
# Usage:
#   make            — build
#   make run        — build and run simulation
#   make demo       — build and run judge demo
#   make test       — build and run integration test
#   make clean      — remove build artefacts
#
# QNX cross-compile: replace CC=gcc with CC=qcc, add -Vgcc_ntoaarch64le
# =============================================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -I./include
LDFLAGS  = -lm

TARGET   = fcw_sim
SRCDIR   = src
BUILDDIR = build

SOURCES  = $(SRCDIR)/main.c                  \
           $(SRCDIR)/sensor_process.c        \
           $(SRCDIR)/digital_twin_process.c  \
           $(SRCDIR)/prediction_engine.c     \
           $(SRCDIR)/warning_system.c        \
           $(SRCDIR)/dash_proc.c

OBJECTS  = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build OK -> $(BUILDDIR)/$(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(BUILDDIR)/$(TARGET)

demo: all
	./$(BUILDDIR)/$(TARGET) --demo

test: all
	./$(BUILDDIR)/$(TARGET) --test

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(TARGET)
	@echo "Cleaned."

.PHONY: all run demo test clean