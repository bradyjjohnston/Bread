# 🍞 SALT - Slice And Loaf Telemetry

> A modular home automation and environmental sensing platform, developed in collaboration with **EE4737** and the open-source [BREAD] project.

---

## Overview

BREAD is a modular embedded systems architecture built around a central **Loaf** board and hot-swappable **Slice** modules. Each slice handles its own sensing or control functionality and communicates back to the loaf over a shared protocol (UART). Together, they form a capable at-home weather station and automation hub.

---

## Architecture

```
┌────────────────────────────────────────┐
│              Loaf (Main Board)         │
│   Central controller with female conn. │
│   handles aggregation, display, logic  │
└────────┬────────┬────────┬─────────────┘
         │        │        │
    ┌────┴──┐ ┌───┴──┐ ┌───┴──┐
    │Slice 1│ │Slice2│ │Slice3│  ...
    │SENSE  │ │ ACT. │ │ OPT. │
    └───────┘ └──────┘ └──────┘
```

- **Loaf** — the main board with female connectors, acts as the central hub
- **Slices** — individual modules that plug into the loaf via edge connectors and expose sensor data or control outputs over UART/I2C

---

## Features

- 🌡️ **Environmental monitoring** — temperature, humidity, pressure (BME280)
- 🌐 **Weather data integration** — pulls external weather API data
- 📡 **Modular slice communication** — UART packet protocol between slices and loaf
- 🔌 **Plug-and-play expansion** — add new slices without redesigning the core system
- 🏠 **Home automation ready** — designed to grow into broader automation use cases

---

## Hardware

| Component | Role |
|-----------|------|
| LOAF | Sensor slice microcontroller |
| Sense Slice | temp / humidity / pressure sensor / boot sensor |
| Actuation Slice | salt distribution motor control |
| Optics Slice | camera processing and control |
| Power Slice | power outage detection and power distribution |

---

## Communication Protocol

Slices send formatted UART packets to the loaf via DMA when requested by the master MCU. Each packet includes:

- Formatted sensor data

---

## Getting Started

> ⚠️ Hardware required. Software setup instructions coming soon.

1. Clone this repo
2. Open in **STM32CubeIDE**
3. Flash the slice firmware to your Nucleo F303K8
4. Connect the slice to the loaf board via the edge connector
5. Monitor output over UART at `115200` baud

---

## Project Context

This system was developed as part of **EE4737** coursework and builds on the open-source BREAD framework. The goal is a practical, extensible platform for real-world embedded sensing and home automation experimentation.

---

## License

MIT — see `LICENSE` for details.
