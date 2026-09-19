# TCL TAC-PRO12PEC CMD 0x0A Energy Decode Candidate

This test build keeps the Home Assistant entities unchanged and adds a conservative decoder to the log only.

The public AC-hack reverse engineering documents packed-BCD accumulated compressor/outdoor-unit energy when CMD 0x0A payload[2] is 0x0C. This TAC-PRO12PEC returns 0x0D. Because the six energy-field bytes in the captured 0x0D frames are still valid packed BCD and change plausibly, this build applies the documented 0x0C byte layout to 0x0D **as a candidate only**.

Expected log example:

    [TCL-ENERGY]: Energy candidate (0x0C layout applied to flag 0x0D): 6.535204 kWh ...

No HA sensor is published from this candidate yet. Action v2, existing sensors, UART/control behavior, and estimated Power/Energy entities are unchanged.
