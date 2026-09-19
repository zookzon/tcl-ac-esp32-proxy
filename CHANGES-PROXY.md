# Proxy-mode changes

- Added optional second UART (`dongle_uart_id`).
- Added transparent factory-dongle -> AC byte forwarding.
- Added transparent AC -> factory-dongle byte forwarding.
- Added frame reconstruction/logging for factory dongle traffic.
- Added idle-time arbitration for ESPHome-generated commands.
- Added one-frame local TX queue; newest pending HA command wins.
- Disabled normal periodic ESPHome polling while factory-dongle traffic is active.
- Added fallback ESPHome polling after prolonged factory-dongle silence.
- Added configuration validation: `proxy_mode: true` requires `dongle_uart_id`.
- Updated sample ESP32-C3 YAML for GPIO3/4 (AC) + GPIO5/6 (factory dongle).
- Raw Packet Logging switch now also toggles proxy traffic logging.
- Added `PROXY-MODE.md` wiring/test guide.
