# A5 ACK-only Debug V2

Purpose: diagnose the previous build where no TCL-A5 lines appeared.

Changes only in A5 test path:
- Runtime banner emitted from loop() after logger is online.
- 5-second heartbeat with raw RX byte count, valid frame count, CRC failures, ACK TX count.
- RX activity counter before A5 framing/parser.
- Valid-frame and ACK logs use `A5-TEST` tag.
- A5 remains 115200 8N1, GPIO3 TX / GPIO4 RX.
- Legacy BB/control remains blocked in ACK-only mode.
- Factory dongle TX must be disconnected during this TX test.

Expected even with no AC traffic:
`[A5-TEST] RUNTIME START ...`
`[A5-TEST] HEARTBEAT: raw_rx=0 ...` every 5 seconds.
