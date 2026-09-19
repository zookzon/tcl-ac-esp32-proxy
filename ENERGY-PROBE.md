# TCL CMD 0x0A Energy Probe

Baseline: `tclac-c3-action-v2-sensor-group-v2.zip`.

This build does not change Action v2 or the existing Home Assistant sensors.
It adds a read-only protocol probe for TCL command `0x0A`.

Sequence:
1. Normal `0x04` GET/status exchange completes.
2. Firmware waits 180 ms.
3. Sends `BB 00 01 0A 03 05 00 00 B6`.
4. Parser accepts the 51-byte `0x0A` response and logs the entire raw frame.
5. Probe is rate-limited to once every 30 seconds.

Look for log tags `TCL-ENERGY`.

No kWh entity is decoded in this build. We first need a real TAC-PRO12PEC
response to verify whether its payload uses the packed-BCD counter mapping
reported by public TCL protocol reverse engineering.
