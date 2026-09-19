# A5 Takeover Phase 2

Experimental TAC-PRO12PEC build based on the proven A5 framing used by codypendant/aciq-minisplit-protocol.

Adds:
- mandatory ACK (~50 ms)
- 17-byte clock reply to 10/10 request
- RSSI heartbeat every ~60 s
- one explicit test command button: **A5 Test Setpoint 25C**

The test command sends the repo-proven dual-record setpoint form: field 0x02 = 2500 centi-C and parameter p0x27 = 77 F.

**Factory dongle TX must be disconnected for this build.**

Expected logs use `A5-TEST`, `TCL-A5`, and `A5-PHASE2`.
