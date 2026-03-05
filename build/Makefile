# =============================================================================
# Makefile — Digital Twin FCW System (QNX IPC Multi-Process Edition)
# =============================================================================
#
# Targets:
#   make              — build everything (4 IPC processes + legacy sim)
#   make linux        — same as make (explicit)
#   make qnx          — cross-compile for QNX Neutrino (aarch64)
#   make run_sim      — run legacy single-process simulation
#   make demo         — run --demo mode
#   make test         — run --test mode
#   make clean        — remove all build artefacts
#
# QNX cross-compile example:
#   make qnx CC=qcc CFLAGS_EXTRA="-Vgcc_ntoaarch64le"
# =============================================================================

CC          = gcc
CFLAGS      = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L \
              -I./include $(CFLAGS_EXTRA)
LDFLAGS     = -lm -lpthread -lrt

ifeq ($(CC),qcc)
LDFLAGS     = -lm
CFLAGS     += -D__QNX__
endif

SRCDIR      = src
BUILDDIR    = build

# ─── Common module objects ────────────────────────────────────────────────── #
OBJ_SENSOR  = $(BUILDDIR)/sensor_process.o
OBJ_TWIN    = $(BUILDDIR)/digital_twin_process.o
OBJ_PREDICT = $(BUILDDIR)/prediction_engine.o
OBJ_WARNING = $(BUILDDIR)/warning_system.o
OBJ_DASH    = $(BUILDDIR)/dash_proc.o

# ─── Process entry objects ────────────────────────────────────────────────── #
OBJ_SENSOR_MAIN = $(BUILDDIR)/sensor_proc_main.o
OBJ_TWIN_MAIN   = $(BUILDDIR)/twin_proc_main.o
OBJ_ALERT_MAIN  = $(BUILDDIR)/alert_proc_main.o
OBJ_DASH_MAIN   = $(BUILDDIR)/dash_proc_main.o
OBJ_MAIN        = $(BUILDDIR)/main.o

# ─── Binaries ─────────────────────────────────────────────────────────────── #
BIN_SENSOR  = $(BUILDDIR)/sensor_proc
BIN_TWIN    = $(BUILDDIR)/twin_proc
BIN_ALERT   = $(BUILDDIR)/alert_proc
BIN_DASH    = $(BUILDDIR)/dash_proc_bin
BIN_SIM     = $(BUILDDIR)/fcw_sim

# =============================================================================
all: $(BUILDDIR) \
     $(BIN_SENSOR) $(BIN_TWIN) $(BIN_ALERT) $(BIN_DASH) $(BIN_SIM)
	@echo ""
	@echo "========================================================"
	@echo "  BUILD COMPLETE"
	@echo ""
	@echo "  IPC processes:"
	@echo "    $(BIN_SENSOR)  (priority 30)"
	@echo "    $(BIN_TWIN)    (priority 25)"
	@echo "    $(BIN_ALERT)   (priority 28)"
	@echo "    $(BIN_DASH)    (priority 10)"
	@echo ""
	@echo "  Legacy simulation:"
	@echo "    $(BIN_SIM)"
	@echo ""
	@echo "  Run multi-process : ./start_fcw.sh"
	@echo "  Run simulation    : make run_sim"
	@echo "  Run demo          : make demo"
	@echo "  Run tests         : make test"
	@echo "========================================================"

linux: all

qnx:
	$(MAKE) CC=qcc CFLAGS_EXTRA="-Vgcc_ntoaarch64le -D__QNX__"

# =============================================================================
# IPC Process binaries
# =============================================================================

$(BIN_SENSOR): $(OBJ_SENSOR_MAIN) $(OBJ_SENSOR)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Linked -> $@"

$(BIN_TWIN): $(OBJ_TWIN_MAIN) $(OBJ_TWIN) $(OBJ_PREDICT) $(OBJ_WARNING)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Linked -> $@"

$(BIN_ALERT): $(OBJ_ALERT_MAIN) $(OBJ_WARNING)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Linked -> $@"

$(BIN_DASH): $(OBJ_DASH_MAIN) $(OBJ_DASH)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Linked -> $@"

# =============================================================================
# Legacy single-process simulation
# =============================================================================

$(BIN_SIM): $(OBJ_MAIN) $(OBJ_SENSOR) $(OBJ_TWIN) $(OBJ_PREDICT) \
            $(OBJ_WARNING) $(OBJ_DASH)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Linked -> $@"

# =============================================================================
# Object file compilation
# =============================================================================

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# =============================================================================
# Run targets
# =============================================================================

run_sim: $(BIN_SIM)
	./$(BIN_SIM)

demo: $(BIN_SIM)
	./$(BIN_SIM) --demo

test: $(BIN_SIM)
	./$(BIN_SIM) --test

clean:
	rm -rf $(BUILDDIR)
	@echo "Cleaned."

.PHONY: all linux qnx run_sim demo test clean