# Copyright and Source Provenance

Copyright © 2026 zookzon for original project-specific material identified below.

This notice is intentionally scoped. It does **not** claim ownership of third-party code, upstream TCL ESPHome code, or protocol research authored by others.

## Original project-specific work

Subject to the provenance qualifications below, the project claims copyright in its original expression and implementation work created for the TCL TAC-PRO12PEC project, including:

- the project-specific ESP32-C3 **dual-UART factory-dongle mediation implementation**;
- the project-specific architecture in which the ESP32-C3 remains the **permanent A5 owner** toward the AC while the factory TCL/WBR1 dongle is attached to a separate UART;
- project-specific factory-dongle coexistence, routing/mediation, and related safety logic;
- TAC-PRO12PEC-specific integration, warm-rejoin/state-synchronization implementation added by this project;
- project-specific Home Assistant integration behavior, diagnostics, testing work, and documentation where independently authored here;
- original wiring documentation and project-specific test methodology.

Copyright protects the original source-code and documentation expression. This notice does not assert exclusive rights over the general idea of using two UARTs, the A5 protocol itself, facts discovered by protocol analysis, or third-party techniques.

## Upstream TCL component lineage

Development began from:

- `thedesp/tclac`, commit `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`
- which identifies `I-am-nightingale/tclac` as its original project.

Code inherited from, copied from, or substantially based on that lineage remains subject to the rights of its respective authors. This project's copyright notice does not replace or supersede those rights.

Because a repository-root license grant for that lineage has not yet been verified, no blanket project license is asserted for the complete `components/tclac/` source tree.

## A5 protocol research

`codypendant/aciq-minisplit-protocol` was used as an important A5 protocol research reference and is published under the MIT License.

Protocol facts are not claimed here as original merely because this project implements or independently confirms them. Any source copied or substantially derived from MIT-licensed code must retain the applicable MIT copyright and permission notice.

See `THIRD_PARTY_NOTICES.md` and `ACKNOWLEDGEMENTS.md`.

## Mixed-provenance files

Files such as `components/tclac/tclac.cpp`, `tclac.h`, `climate.py`, and `automation.h` may contain both inherited code and project-specific additions.

Accordingly:

1. Copyright © 2026 zookzon applies only to independently authored project-specific additions and modifications.
2. No ownership is claimed over pre-existing upstream portions.
3. No whole-file relicensing is implied by this notice.
4. Source provenance should be audited before a future project-wide license is selected.

## Licensing status

At this stage the repository intentionally has **no blanket project-wide LICENSE**.

A future license may be applied to clearly separable original files or modules after provenance boundaries are documented. That license must not purport to relicense third-party material for which this project does not hold the necessary rights.

## Attribution

For third-party projects and known license information, see:

- `ACKNOWLEDGEMENTS.md`
- `THIRD_PARTY_NOTICES.md`

This file documents project provenance and copyright claims; it is not itself a grant of permission to use third-party code.
