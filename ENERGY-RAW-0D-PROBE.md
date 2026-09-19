# TAC-PRO12PEC CMD 0x0A subtype 0x0D raw probe

This build intentionally removes the speculative `0x0C`-layout kWh candidate for subtype `0x0D`.
It does not change Action v2, climate control, normal status parsing, estimated power/energy entities, UART, or the 30-second / 180-ms energy query timing.

New logs:
- `TCL-ENERGY-RAW`: indexes all 45 payload bytes as P00..P44 (`P02` is subtype/flag `0x0D`).
- `TCL-ENERGY-DELTA`: compares each response with the previous response and lists only changed payload bytes plus elapsed milliseconds.
- `TCL-ENERGY-STATE`: records B38/B39/B40/B45/B46 from the latest normal state packet for correlation only.

No subtype 0x0D field is labeled kWh or watts in this build.
