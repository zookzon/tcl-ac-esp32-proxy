# TCL TAC-PRO12PEC ESPHome controller

Custom ESPHome component for a TCL TAC-PRO12PEC air conditioner connected to
an ESP32-C3 through the unit's proprietary UART interface.

This tree is based on `thedesp/tclac` commit
`9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`, with compatibility, parser,
diagnostic, Home Assistant, and safety changes proven during testing on the
target unit.

Start here:

- [`PROJECT-HANDOFF.md`](PROJECT-HANDOFF.md) — complete Thai handoff document,
  history, architecture, installation, known problems, and remaining work.
- [`PACKET-MAP.md`](PACKET-MAP.md) — RX/TX packet map, formulas, observations,
  and confidence levels.
- [`TEST-PLAN.md`](TEST-PLAN.md) — reproducible logging and reverse-engineering
  procedure for the next maintainer.
- [`TESTING-ELECTAPP.md`](TESTING-ELECTAPP.md) — legacy detailed diagnostic
  notes retained for traceability.
- [`tclac-c3-full.yaml`](tclac-c3-full.yaml) — current ESPHome configuration.

## Safety

The air conditioner's USB-shaped connector is **not USB**. Never connect it to
a computer or USB charger. It carries the TCL proprietary serial interface.

## Current validated build

- ESPHome: 2026.7.4
- Board: ESP32-C3 (`esp32-c3-devkitm-1`)
- UART: GPIO3 TX, GPIO4 RX, 9600 baud, 8 data bits, even parity, 1 stop bit
- Full compile: passed
- RAM: 33.2%
- Flash: 56.0%

No credentials or generated build cache are included in the source archive.

## Factory TCL Dongle + ESP32-C3 Proxy Mode

This project now includes an experimental dual-UART proxy mode so the original TCL Wi-Fi dongle and ESPHome/Home Assistant can coexist without placing two UART TX outputs in parallel. See `PROXY-MODE.md` for wiring, configuration, packet logging, and the first-test procedure.


## HA sensor grouping / energy note (2026-09-17)

- `Outdoor Temperature` is a normal measurement sensor (not a diagnostic entity), so Home Assistant groups it with Indoor Temperature, Current, Power and Energy.
- `Compressor Power (Estimated)` and `Compressor Energy (Estimated)` remain local estimates.
- Research of the TCL Home unofficial integration shows its Today/Yesterday Energy Consumption values are fetched on demand from TCL's cloud API and refreshed hourly; this does not establish a local UART BB energy field.
- A separate reverse-engineered local UART protocol documents CMD 0x0A as a power/status query, but its observed response does not identify a validated kWh counter. Therefore no unverified UART byte is exposed as energy in this build.
