# TCL TAC-PRO12PEC ESPHome A5 Controller + Factory Dongle Mediator

ESPHome firmware for controlling a **TCL TAC-PRO12PEC** with an **ESP32-C3** while retaining the original TCL/WBR1 factory Wi-Fi dongle on a separate UART.

The current tested production architecture uses the **A5 protocol at 115200 baud, 8N1**. The ESP32-C3 remains the permanent A5 owner toward the air conditioner; there is no UART ownership handoff.

> Historical BB/9600/8E1 experiments are retained in this repository for engineering traceability, but they are **not** the current production backend.

## Production architecture

```text
Home Assistant
      |
  ESPHome API
      |
  ESP32-C3
   |      \
   |       \ GPIO6 TX / GPIO5 RX — 115200 8N1
   |        +---------------- Factory TCL/WBR1 dongle
   |
   + GPIO3 TX / GPIO4 RX — 115200 8N1
      |
  TCL TAC-PRO12PEC
```

Production configuration:

- `a5_ack_only: true`
- `proxy_mode: false`
- AC UART: GPIO3 TX / GPIO4 RX, 115200 8N1
- Factory dongle UART: GPIO6 TX / GPIO5 RX, 115200 8N1
- AC read-back is authoritative; Home Assistant state is not optimistically overwritten by commands.
- Factory-dongle coexistence is mediated by the ESP32 instead of electrically paralleling two UART TX outputs.

See [Current Architecture](docs/CURRENT-ARCHITECTURE.md) and [Wiring](docs/WIRING.md).

## Tested hardware / parts

Only the following additional parts are required for the tested build:

| Part | Requirement | Example purchase link |
|---|---|---|
| ESP32 board | **ESP32-C3 Super Mini** | [AliExpress](https://www.aliexpress.com/item/1005005967641936.html) |
| USB pigtails | **USB-A Female + USB-A Male, 4-wire** | [AliExpress](https://www.aliexpress.com/item/1005008997291374.html) |

The links are examples of the parts used for this project, not affiliate links. Equivalent parts may be used, but the USB pigtails must expose all **4 wires**. Do not use 2-wire power-only pigtails. Verify the actual pinout/continuity before connecting anything to the air conditioner.

## Wiring

UART TX and RX must be crossed:

| Device side | ESP32-C3 |
|---|---|
| Factory dongle TX (white) | GPIO5 (RX) |
| Factory dongle RX (red/pink) | GPIO6 (TX) |
| TCL AC TX / D- (white) | GPIO4 (RX) |
| TCL AC RX / D+ (pink) | GPIO3 (TX) |
| 5V | Shared 5V |
| GND | Shared GND |

**Do not connect two TX outputs together.** See [docs/WIRING.md](docs/WIRING.md) for the complete diagram, wire colors, pin layout, and safety notes.

## Compatibility / air-conditioner models

### Fully tested with this repository

- **TCL TAC-PRO12PEC — TESTED / CONFIRMED.** This is the development and production test unit for this repository. The current A5 115200 8N1 firmware, ESP32-C3 control path, Home Assistant behavior, and dual-UART factory-dongle mediator have been tested on this model and are the configuration this project considers fully validated.

### A5-family reference hardware

- **ACiQ ACIQ-K18W-W-32-HP2300 — A5 REFERENCE / NOT TESTED WITH THIS REPOSITORY.** The `codypendant/aciq-minisplit-protocol` project reports hardware-proven A5 115200 8N1 operation on this model with a TCL WBR1 module. Its protocol research is an important reference for this project. Similarity strongly suggests useful compatibility, but this repository's complete firmware and dual-UART mediator have **not** been validated by us on that model.

### Models reported working by the upstream TCL `tclac` lineage

The upstream `I-am-nightingale/tclac` project reports the following models as tested with its TCL component. These reports are useful compatibility leads, but that upstream project primarily represents the older TCL/compatible protocol family and **does not prove that these models use this repository's A5 115200 8N1 protocol or that our dual-UART mediator will work unchanged**:

- Axioma ASX09H1/ASB09H1
- Ballu BSAI-12HN1_15Y
- Ballu Discovery DC BSVI-07HN8
- Ballu Discovery DC BSVI-09HN8
- Ballu Discovery DC BSVI-12HN8
- Daichi AIR20AVQ1/AIR20FV1
- Daichi AIR25AVQS1R-1/AIR25FVS1R-1
- Daichi AIR35AVQS1R-1/AIR35FVS1R-1
- Daichi DA35EVQ1-1/DF35EV1-1
- Dantex RK-12SATI/RK-12SATIE
- Ecostar Radium KVS-RAD09CH
- iFFALCON F1 18
- Royal Clima Gloria Inverter
- Royal Clima Pandora RC-PDC28HN
- Tesla TT27TP61S-0932IAWUV
- TCL ELI ONF 12
- TCL Liferise ONF 09
- TCL TAC-CT09INV/R
- TCL One Inverter TACM-09HRID/E1
- TCL TAC-07CHSA/TPG-W
- TCL TAC-09CHSA/TPG
- TCL TAC-09CHSA/DSEI-W
- TCL TAC-09HRID/E1
- TCL TAC-12CHSA/TPG
- TCL TAC-12CHSA/TPGI
- TCL TAC-XAL24I
- TCL TPG31IHB

The upstream author also warns that even nominally identical AC model numbers can ship with different Wi-Fi/UART hardware or wiring. Therefore **model number alone is not enough to guarantee compatibility**.

For an untested unit, first identify the UART electrically and determine its framing/baud before transmitting. In particular, do not assume that an upstream BB/9600/8E1-compatible model is automatically compatible with this project's A5/115200/8N1 production backend.

**Compatibility labels used here:**

- **TESTED / CONFIRMED** — tested with this repository's firmware/hardware arrangement.
- **A5 REFERENCE** — documented A5 hardware from a protocol-reference project, but not tested by us with the complete firmware.
- **UPSTREAM-REPORTED** — reported working by an upstream TCL project; possible candidate only, not a compatibility guarantee for this A5 build.

## Current features

The production build provides:

- Climate modes: OFF, AUTO, COOL, HEAT, DRY, FAN_ONLY
- Fan modes: AUTO, QUIET, LOW, MIDDLE, MEDIUM, HIGH, FOCUS, DIFFUSE
- Swing: OFF, VERTICAL, HORIZONTAL, BOTH
- Presets exposed by the current configuration
- A5 read-back/control for supported AC features
- Indoor and outdoor temperature reporting
- AC-reported Input Power
- Compressor Target and Compressor Actual sensors
- Total, Today, Yesterday, and Monthly Energy derived by integrating AC-reported Input Power
- Wi-Fi diagnostics
- Native ESPHome uptime sensor
- Configurable 0.5 °C / 1.0 °C target-temperature step

The source code is the final authority for implemented behavior. Experimental/raw protocol fields remain diagnostic until validated.

## Energy behavior

`Input Power` is decoded from the AC-reported A5 field used by this build. Energy entities integrate that power locally using ESPHome's trapezoid integration:

- **Total Energy** — session total; resets after ESP32 reboot.
- **Today Energy** — persisted and reset at local midnight.
- **Yesterday Energy** — last completed calendar day, persisted.
- **Monthly Energy** — persisted and reset on the first day of the month.

These values are local integrations of AC-reported power, not utility-grade metering. Compare against an external meter before relying on them for billing or precision energy analysis.

## Installation

1. Clone or download this repository.
2. Copy `secrets.example.yaml` to your ESPHome secrets configuration and replace all placeholders.
3. Review [docs/WIRING.md](docs/WIRING.md) before connecting hardware.
4. Use `tclac-c3-full.yaml` as the production ESPHome configuration.
5. Validate/compile with a compatible ESPHome version.
6. Flash the ESP32-C3 and verify AC read-back before relying on automations.
7. Verify the factory TCL dongle independently after the ESP32 side is stable.

The tested source is vendored locally under `components/tclac/`; no runtime download of the TCL component is required.

## Repository map

- [`tclac-c3-full.yaml`](tclac-c3-full.yaml) — production ESPHome configuration
- [`components/tclac/`](components/tclac/) — vendored custom TCL component
- [`docs/WIRING.md`](docs/WIRING.md) — production wiring
- [`docs/CURRENT-ARCHITECTURE.md`](docs/CURRENT-ARCHITECTURE.md) — current data path and ownership model
- [`docs/PROTOCOL-STATUS.md`](docs/PROTOCOL-STATUS.md) — protocol evidence/status matrix
- [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) — troubleshooting
- [`ACKNOWLEDGEMENTS.md`](ACKNOWLEDGEMENTS.md) — upstream and research credits
- [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) — third-party licensing notes

Additional root-level engineering notes are retained as development history. Some describe earlier BB/proxy experiments and must not be treated as production instructions.

## Protocol evidence policy

Protocol knowledge is separated into **CONFIRMED**, **OBSERVED**, **HYPOTHESIS**, and **UNKNOWN**. A plausible byte value is not assigned a meaning without evidence. For current protocol status, see [docs/PROTOCOL-STATUS.md](docs/PROTOCOL-STATUS.md).

## Safety

The air conditioner's USB-shaped connector is **not a standard USB interface**. Do not connect it to a PC or USB charger.

Power the AC down before changing wiring. Verify the pinout of your exact ESP32-C3 board revision and TCL hardware. The documented wiring is for the tested TAC-PRO12PEC setup; other TCL models or factory-dongle revisions may differ.

## Upstream and protocol research

This project began from `thedesp/tclac` commit `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`, whose README identifies `I-am-nightingale/tclac` as the original project. The A5 work also used `codypendant/aciq-minisplit-protocol` as an important protocol reference.

See [ACKNOWLEDGEMENTS.md](ACKNOWLEDGEMENTS.md) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License status

**Project-original portions © 2026 zookzon are available under the MIT License**, including independently authored project-specific work such as the dual-UART factory-dongle mediation implementation where the project owns the copyright. You may use, modify, distribute, and build upon those portions under the MIT terms while preserving the applicable copyright and permission notice.

This is not a blanket relicensing of the entire repository. Upstream and third-party portions remain subject to their respective copyrights and licenses. See [COPYRIGHT.md](COPYRIGHT.md) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Tested scope

Current production work is validated specifically against:

- TCL TAC-PRO12PEC
- ESP32-C3 Super Mini
- USB-A Female/Male 4-wire pigtails
- The tested factory TCL/WBR1 dongle arrangement

Compatibility with other models is not guaranteed.
