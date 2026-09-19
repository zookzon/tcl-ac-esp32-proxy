# Current Architecture

## Production data path

```text
Home Assistant
     |
 ESPHome API
     |
 ESP32-C3
  |      \
  |       \ UART2: GPIO6 TX / GPIO5 RX, 115200 8N1
  |        +---------------- Factory TCL/WBR1 dongle
  |
  + UART1: GPIO3 TX / GPIO4 RX, 115200 8N1
     |
 TCL TAC-PRO12PEC indoor unit
```

The ESP32 is the permanent A5 owner toward the AC. It performs required A5 service behavior, decodes AC state, sends HA commands, mirrors AC traffic to the factory dongle, and mediates only known factory-dongle request classes. There is no TX handoff.

`a5_ack_only: true` selects this production backend. `proxy_mode: false` means the old BB proxy backend is not active.

## State authority

AC `0C 0C` reports are authoritative for climate/state publication. Commands do not optimistically overwrite HA state.

## Boot/rejoin

After an ESP32-only reboot, a captured factory-style INIT is sent. The first valid state report ends retries; a one-time state/parameter query follows to refill persistent/read-back state.
