# A5 Mediator V5

Purpose: keep ESP32 as permanent A5 owner/controller of the AC while allowing the Factory TCL/WBR1 dongle to complete evidence-backed A5 startup/query/control transactions on its separate UART.

## Wiring (unchanged)
- AC: ESP32 TX GPIO3 / RX GPIO4, 115200 8N1
- Factory Dongle: ESP32 TX GPIO6 / RX GPIO5, 115200 8N1
- Common GND; no wiring changes.

## V5 routing
Factory Dongle -> AC forwards only valid CRC A5 type 0x21 frames in these proven/externally corroborated classes:
- payload `00 00 01` (startup/init frame)
- payload prefix `0B 0B` (state/parameter query family; `FF FF` observed)
- payload prefix `0A 0A` (control command)

Factory Dongle type 0x23 remains suppressed toward AC to avoid duplicate ACK/service ownership. Unknown classes are logged and suppressed.

Every AC-originated byte remains mirrored to the Factory Dongle, so genuine AC responses to forwarded Dongle requests return to the Dongle. ESP32's own generated ACK/Clock/RSSI frames are not mirrored.

There is NO handoff. Existing A5 Production climate/state logic is retained.
