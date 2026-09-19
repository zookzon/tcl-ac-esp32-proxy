# A5 Owner + Bridge — Permanent NO HANDOFF V3

Base: a5_dual_owner_v1.

This build intentionally keeps the exact pre-handoff OWNER+BRIDGE behavior permanently:
- AC UART: ESP TX GPIO3 / RX GPIO4, 115200 8N1.
- Factory Dongle UART: ESP TX GPIO6 / RX GPIO5, 115200 8N1.
- ESP32 remains the A5 owner for the AC for the entire runtime.
- Every AC-originated byte is mirrored to the Factory Dongle.
- Factory Dongle frames are continuously parsed.
- Dongle type 0x23/service responses are suppressed from AC.
- Proven 0A0A user commands remain eligible for forwarding exactly as in V1.
- There is NO timed handoff and NO transition to transparent proxy.

Expected runtime banner:
A5 OWNER+BRIDGE PERMANENT / NO HANDOFF
