# A5 Dual Owner / Factory Dongle Bridge V1

Base: user A5 Production source.

Wiring is fixed:
- AC: ESP TX GPIO3, ESP RX GPIO4
- Factory Dongle: ESP TX GPIO6 -> Dongle RX; ESP RX GPIO5 <- Dongle TX
- Both UARTs: 115200 8N1 continuously. No BB phase switch.

Architecture:
- ESP32 remains the A5 protocol owner toward the AC (ACK/clock/RSSI/state/climate unchanged).
- Every AC-originated byte is mirrored to Factory Dongle.
- Factory Dongle frames are parsed and CRC checked.
- Dongle type 0x23 ACK/service replies are suppressed to prevent duplicate replies to AC.
- Only validated command class type 0x21 payload 0A0A is forwarded intact to AC.
- Unknown Dongle frame classes are suppressed and logged; no guessed semantics.

Expected log tag: A5-BRIDGE.
