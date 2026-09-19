# Action v2 — machine-operation based

Only the Climate `action` decision was changed in this build.

Unchanged intentionally:
- Dual-Phase Proxy / Phase 1 / Phase 2 timing and UART behavior
- proxy forwarding/control logic
- existing TCL-OP diagnostics, including the current `power=` diagnostic
- packet decoding and sensors

## Machine-operation evidence used

Running-like packets observed:
- B38 > 0
- B39 > 0
- B40 = 0x8A
- B46 > 0

Stopped/off-like packets observed:
- B38 = 0
- B39 = 0
- B40 = 0x80
- B46 = 0

B40 is used as the primary state indicator. B38/B39/B46 are supporting evidence.

## Action mapping

- OFF -> OFF
- COOL + compressor active -> COOLING; otherwise IDLE
- HEAT + compressor active -> HEATING; otherwise IDLE
- DRY + compressor active -> DRYING; otherwise IDLE
- FAN_ONLY -> FAN
- AUTO -> IDLE for now, because the captured machine-side bytes do not yet prove cooling-vs-heating direction in AUTO.

The old target-temperature/current-temperature comparison is no longer used for COOL/HEAT Action.
