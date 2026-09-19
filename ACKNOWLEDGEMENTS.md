# Acknowledgements

This project exists because of earlier TCL ESPHome work and later A5 protocol research. The sources below are credited according to how they were used.

## TCL ESPHome / BB protocol lineage

### I-am-nightingale/tclac

Original TCL ESPHome component and the upstream project referenced by the later `thedesp/tclac` repository.

- Repository: https://github.com/I-am-nightingale/tclac
- Protocol family: TCL `0xBB`, typically 9600 8E1.
- Role here: historical/original component lineage and implementation reference.

### thedesp/tclac

The starting codebase used during development of this project.

- Repository: https://github.com/thedesp/tclac
- Upstream commit used as the project starting point: `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`
- Protocol family: TCL `0xBB`, typically 9600 8E1.
- Role here: ESPHome TCL component starting point, configuration structure, and earlier TCL integration work.
- Its README explicitly credits `I-am-nightingale/tclac` as the original project.

The TAC-PRO12PEC unit tested for this repository does **not** use the production BB backend. Hardware testing showed that the working production protocol is A5 at 115200 8N1.

## A5 protocol research

### codypendant/aciq-minisplit-protocol

A major protocol reference for the TCL WBR1 / A5 family.

- Repository: https://github.com/codypendant/aciq-minisplit-protocol
- Protocol: `0xA5` framing at 115200 8N1.
- Published research includes framing, CRC-16/XMODEM, ACK behavior, clock/RSSI behavior, field mapping, command construction, and hardware-tested ESPHome control.
- License: MIT for that repository's original work.

This repository used that work as an important protocol reference while adapting and validating behavior on a **TCL TAC-PRO12PEC**.

## Work specific to this repository

The following work was developed and/or validated specifically for this project and target setup:

- Adaptation and validation on TCL TAC-PRO12PEC.
- ESP32-C3 implementation.
- Dual-UART architecture.
- Simultaneous factory-dongle coexistence through an ESP32 mediator.
- ESP32-C3 permanent A5 ownership toward the AC.
- Factory-dongle state mirroring/mediation behavior.
- Warm-rejoin/state-sync work.
- Home Assistant behavior and additional diagnostics.
- Energy, temperature, compressor, and operation investigations documented in this repository.
- Production wiring and GPIO mapping for the tested setup.

Where a behavior was learned from upstream research rather than independently discovered here, the project documentation should say so. Where it was independently observed or confirmed on TAC-PRO12PEC hardware, it should be described as independently confirmed rather than claimed as an original discovery.

## Evidence policy

Protocol statements should be classified as **CONFIRMED**, **OBSERVED**, **HYPOTHESIS**, or **UNKNOWN**. Similarity to another TCL/WBR1 unit is useful evidence, but it is not by itself proof that every field or behavior is identical on TAC-PRO12PEC.
