# A5 Owner+Bridge Diagnostic V4

Purpose: capture complete valid Factory Dongle A5 frames without changing routing behavior.

- AC UART: ESP TX GPIO3 / RX GPIO4, 115200 8N1
- Factory Dongle UART: ESP TX GPIO6 / RX GPIO5, 115200 8N1
- Same Owner+Bridge arbitration as V3.
- No handoff.
- New log tag: `A5-DONGLE-HEX`
- Every CRC-valid Factory Dongle frame is printed in full HEX before the existing arbitration decision.

This build is diagnostic only. It intentionally does not invent a response for payload 0000.
