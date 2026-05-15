# Digital Twin Based Forward Collision Warning System
### QNX Q-eHACK Hackathon | Raspberry Pi 4 | QNX RTOS

---

## Overview

A real-time **Forward Collision Warning (FCW) system** implemented using a
**Digital Twin architecture** on QNX RTOS. A sensor process reads obstacle
distance every 100 ms. A Digital Twin engine mirrors the physical state and
predicts collisions using a Time-To-Collision algorithm. If collision risk is
detected, hardware alerts (LED/buzzer) and a live dashboard fire immediately.

This is a prototype of the same architecture used in production automotive
ADAS systems — QNX powers ECUs in BMW, Audi, Ford, and General Motors.

---

## System Architecture

```
  HC-SR04 Sensor / Simulated Input
           |
           v
  [ sensor_proc ]  Priority 30, SCHED_FIFO
    Reads distance every 100 ms
    Writes to shared fcw_state_t
           |  MsgSend (QNX IPC)
           v
  [ twin_proc ]    Priority 25, SCHED_FIFO
    Smoothed distance (5-reading average)
    Predicted distance (1 s ahead)
    Anomaly detection (spike filter)
           |  TTC Algorithm
           v
  [ alert_proc ]   Priority 28, SCHED_FIFO  (highest — preempts twin on event)
    QNX Pulse received on collision event
    Drives LED + Buzzer via GPIO
           |
           v
  [ dash_proc ]    Priority 10, SCHED_RR
    Reads shared memory every 500 ms
    Renders live dashboard + system_log.txt
```

---

## QNX RTOS Concepts Demonstrated

| Concept                  | Implementation                                      |
|--------------------------|-----------------------------------------------------|
| Microkernel Architecture | 4 separate user-space processes, IPC via kernel     |
| Process Management       | 4 QNX processes with explicit SCHED_FIFO priorities |
| Thread Management        | 2–3 threads per process with pthread + SchedSet()   |
| IPC — MsgSend/Receive    | sensor_proc → twin_proc synchronous message passing |
| IPC — QNX Pulse          | twin_proc → alert_proc async collision notification  |
| IPC — Shared Memory      | fcw_state_t region via shm_open + mmap              |
| Real-Time Scheduling     | SCHED_FIFO for RT tasks, SCHED_RR for dashboard     |
| Resource Management      | GPIO via QNX resource manager (/dev/fcw_sensor)     |
| Real-Time Monitoring     | ClockCycles() latency measurement, live dashboard   |

---

## Project Structure

```
digital-twin-fcw/
├── src/
│   ├── main.c                  Orchestrator (--demo, --test flags)
│   ├── sensor_process.c        MODULE 1 — Sensor simulation
│   ├── digital_twin_process.c  MODULE 2 — Digital Twin engine
│   ├── prediction_engine.c     MODULE 3 — TTC algorithm
│   ├── warning_system.c        MODULE 4 — Alert output
│   └── dash_proc.c             MODULE 5 — Dashboard + logging
├── include/
│   └── fcw_types.h             Shared data structures
├── build/
│   └── Makefile
├── docs/
│   ├── dashboard_output/       Auto-captured screenshots
│   │   ├── SAFE_step01.txt
│   │   ├── WARNING_step02.txt
│   │   └── CRITICAL_step04.txt
│   └── example_output.md
├── system_log.txt              Auto-generated runtime log
└── README.md
```

---

## Hardware Required

| Component              | Purpose                          |
|------------------------|----------------------------------|
| Raspberry Pi 4 (2GB+)  | Main compute platform, QNX RTOS  |
| HC-SR04 Ultrasonic     | Distance measurement (2–400 cm)  |
| Red/Yellow LED + 220Ω  | CRITICAL / WARNING visual alert  |
| Piezo Buzzer (5V)      | Audible alert (1 Hz / 5 Hz)      |
| HDMI Monitor           | Live dashboard display           |
| Breadboard + wires     | GPIO connections                 |

---

## Build & Run

```bash
# Build
make -f build/Makefile

# Run full simulation
make -f build/Makefile run

# Run judge demo (4-stage scenario)
make -f build/Makefile demo

# Run integration test (3/3 pipeline verification)
make -f build/Makefile test
```

---

## Dashboard Features

- **Color-coded alert banner** — Green (SAFE) / Yellow (WARNING) / Red (CRITICAL)
- **Digital Twin output** — Sensor distance, vehicle speed, predicted distance, TTC, anomaly flag
- **ASCII distance bar** — Visual distance shrinks in real time
- **Distance trend graph** — Time-series distance chart with color-coded markers
- **System Health Monitor** — Uptime, event counts, anomaly count
- **Event log** — Scrolling last-6-steps history
- **system_log.txt** — Timestamped log: `HH:MM:SS | dist | speed | predicted | RISK`
- **Auto-screenshots** — Saved to `docs/dashboard_output/` on each alert change

---

## System Log Format

```
# Time     | Distance  |    Speed  | Predicted | Risk
10:01:01 |  87.50 m |  72.0 km/h |   62.50 m | SAFE
10:01:02 |  75.00 m |  72.0 km/h |   50.00 m | WARNING
10:01:03 |  25.75 m |  66.6 km/h |    2.25 m | CRITICAL
```

---

## Innovation Highlights

1. **True Digital Twin** — runs a predictive model independently of sensor; computes divergence metric
2. **Predictive FCW** — warns 1–2 seconds *before* threshold crossing, not after
3. **Anomaly Detection** — 5-reading history buffer filters sensor spikes; anomaly_flag exposed in dashboard
4. **Graduated Alerts** — 3-level severity system (SAFE → WARNING → CRITICAL) with distinct hardware patterns
5. **Industry Alignment** — FCW is an EU-mandated NCAP safety feature; QNX is the production RTOS for ADAS ECUs

---

## Team

| Member   | Role                                              |
|----------|---------------------------------------------------|
| Member A | QNX Lead — processes, IPC, scheduling             |
| Member B | Algorithm Lead — TTC engine, prediction, twin     |
| Member C | Hardware & Dashboard — sensor, display, logging   |

---

TEAM :
Shraddha Pratap Khanapurkar|
Snehal Hazare|
S.Koushik|


*QNX Q-eHACK Hackathon Submission*
