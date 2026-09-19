# TAC-PRO12PEC HEAT transaction diagnostic

This build continues from Energy Raw 0x0D Probe. It does **not** change the HEAT mapping or control packet.

Confirmed against the original `thedesp/tclac` implementation:
- SET HEAT low nibble = `0x01`
- power field contribution = `0x04`
- STATUS HEAT mask remains the existing `MODE_HEAT` decode.

Added log tag: `TCL-HEAT`.

When HEAT is selected, the firmware logs the complete outgoing CMD `0x03` frame, then correlates valid CMD `0x03`/`0x04` frames for about 15 seconds.

Collect log lines containing:
- `TCL-HEAT`
- `Valid RX`
- climate state immediately after selecting HEAT

All Energy 0x0D raw probing, Action v2, existing power diagnostics, HA entities, UART settings, and proxy infrastructure are unchanged.
