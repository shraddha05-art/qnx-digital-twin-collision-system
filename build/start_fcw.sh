#!/bin/bash
# =============================================================================
# start_fcw.sh — Launch all 4 FCW IPC processes
# =============================================================================
#
# STARTUP ORDER (critical):
#   1. alert_proc  — creates QNX channel first, publishes chid to shared memory
#   2. sensor_proc — creates shared memory, starts sensor loop
#   3. twin_proc   — attaches to shm, connects to alert_proc channel
#   4. dash_proc   — attaches last; lowest priority, display only
#
# Press Ctrl+C to stop all processes cleanly.
# =============================================================================

BUILDDIR="./build"
PIDS=()

# ── Check binaries exist ─────────────────────────────────────────────────────
for bin in sensor_proc twin_proc alert_proc dash_proc_bin; do
    if [ ! -f "$BUILDDIR/$bin" ]; then
        echo "[ERROR] $BUILDDIR/$bin not found — run 'make' first"
        exit 1
    fi
done

echo "========================================================"
echo "  Digital Twin FCW — Multi-Process IPC Startup"
echo "========================================================"
echo ""

# ── Clean stale shared memory ────────────────────────────────────────────────
[ -f /dev/shm/fcw_shared_memory ] && rm -f /dev/shm/fcw_shared_memory && \
    echo "  [INFO] Removed stale shared memory"

# ── Graceful shutdown handler ────────────────────────────────────────────────
cleanup() {
    echo ""
    echo "  [INFO] Shutting down all FCW processes..."
    for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null; done
    wait
    echo "  [INFO] All processes stopped."
    exit 0
}
trap cleanup SIGINT SIGTERM

# ── 1. alert_proc first — must create its channel before twin_proc starts ───
echo "  [1/4] Starting alert_proc  (priority 28)..."
$BUILDDIR/alert_proc &
PIDS+=($!)
sleep 0.2

# ── 2. sensor_proc — creates shared memory ───────────────────────────────────
echo "  [2/4] Starting sensor_proc (priority 30)..."
$BUILDDIR/sensor_proc &
PIDS+=($!)
sleep 0.2

# ── 3. twin_proc — attaches, connects to alert channel ───────────────────────
echo "  [3/4] Starting twin_proc   (priority 25)..."
$BUILDDIR/twin_proc &
PIDS+=($!)
sleep 0.1

# ── 4. dash_proc last — lowest priority, display only ────────────────────────
echo "  [4/4] Starting dash_proc   (priority 10)..."
$BUILDDIR/dash_proc_bin &
PIDS+=($!)

echo ""
echo "  All 4 processes running."
echo "  PIDs: ${PIDS[*]}"
echo "  Press Ctrl+C to stop."
echo "========================================================"
echo ""

wait
echo ""
echo "  All FCW processes have exited."