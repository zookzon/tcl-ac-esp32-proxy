# TCL TAC-PRO12PEC ESPHome A5 Controller + Factory Dongle Mediator

ESPHome firmware for controlling a **TCL TAC-PRO12PEC** with an **ESP32-C3** while retaining the original TCL/WBR1 Wi-Fi dongle on a second UART.

This project began from `thedesp/tclac` commit `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be` and evolved through hardware testing and protocol reverse engineering on the target unit.

> **Important:** the current production backend is **A5 at 115200 8N1**. Older BB/9600 findings are engineering history, not the production configuration.

## Current architecture

- ESP32-C3 -> AC: GPIO3 TX / GPIO4 RX, 115200 8N1.
- ESP32-C3 -> factory dongle: GPIO6 TX / GPIO5 RX, 115200 8N1.
- ESP32 remains the permanent A5 protocol owner toward the AC.
- Required ACK/clock/RSSI and warm-rejoin behavior are implemented.
- AC state is mirrored to the factory dongle.
- Home Assistant state is based on AC read-back, not optimistic command state.

See [Current architecture](docs/CURRENT-ARCHITECTURE.md) and the [wiring guide](docs/WIRING.md).

## Wiring

The tested setup uses two independent UARTs. UART TX/RX must be crossed:

- Factory dongle TX -> ESP32 GPIO5 (RX)
- Factory dongle RX <- ESP32 GPIO6 (TX)
- TCL AC TX / D- -> ESP32 GPIO4 (RX)
- TCL AC RX / D+ <- ESP32 GPIO3 (TX)
- 5V and GND are shared between the AC, ESP32-C3 and factory dongle.

**See the complete ASCII diagram, wire colors and mapping table in [docs/WIRING.md](docs/WIRING.md).**

## Features

Climate control supports OFF/AUTO/COOL/HEAT/DRY/FAN_ONLY, target temperature, fan, swing and presets. Native A5 read-back controls include Beep, Display Light, Drying and Health. Sensors include AC-reported Input Power, compressor target/actual, indoor/outdoor/coil temperatures, energy accumulators, Wi-Fi diagnostics and formatted uptime.

Uptime publishes every 60 seconds without zero padding, e.g. `3d 11h 5m 14s`.

## Evidence policy

Protocol knowledge is separated into **CONFIRMED**, **OBSERVED**, **HYPOTHESIS**, and **UNKNOWN**. Unknown bytes/classes are not assigned semantics merely because values look plausible.

## Safety

The TCL connector may look USB-shaped but is a proprietary AC interface. **Do not connect it to a PC or USB charger.** Do not connect two UART TX outputs in parallel. Power down before rewiring.

## Install

1. Copy `secrets.example.yaml` to `secrets.yaml` and replace every placeholder.
2. Review [docs/WIRING.md](docs/WIRING.md) and verify GPIO assignments against your board.
3. Validate and compile with ESPHome.
4. First boot with RAW A5 logging disabled.
5. Verify AC read-back and factory-dongle coexistence before relying on automations.

## Upstream / attribution

Originally based on `thedesp/tclac` at commit `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`, whose README identifies `I-am-nightingale/tclac` as the original project. A5 protocol research from `codypendant/aciq-minisplit-protocol` was also an important reference.

See [Acknowledgements](ACKNOWLEDGEMENTS.md) and [Third-party notices](THIRD_PARTY_NOTICES.md) for the exact attribution and licensing status.

## License status

**No project-wide license is asserted yet.** No root `LICENSE` was found in the two TCL component lineage repositories during the 2026-09-19 audit, while `codypendant/aciq-minisplit-protocol` is MIT licensed. The production source must therefore be audited file-by-file before this repository is assigned a blanket license.

## Tested scope

TCL TAC-PRO12PEC + ESP32-C3 + the tested factory dongle arrangement. Other TCL models/dongle revisions are not guaranteed.
