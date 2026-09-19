# TCL TAC-PRO12PEC — Factory Dongle + ESP32-C3 Proxy Mode

## Goal

Use both control paths at the same time without tying two UART TX outputs together:

- TCL factory Wi-Fi dongle / TCL app
- ESP32-C3 / ESPHome / Home Assistant

The ESP32-C3 is inserted **between** the air conditioner and the factory dongle and acts as a transparent two-UART proxy.

```text
TCL factory dongle            ESP32-C3                     TCL indoor unit
------------------       ------------------             ------------------
TX  -------------------> GPIO5  RX (dongle UART)
RX  <------------------- GPIO6  TX (dongle UART)
                         GPIO4  RX (AC UART) <----------- TX
                         GPIO3  TX (AC UART) -----------> RX
GND --------------------------- common GND ---------------- GND
```

Do not connect the factory-dongle TX and ESP32 TX directly in parallel to the AC RX line.

## Important: the TCL connector is not USB

The upstream `thedesp/tclac` project documents a USB-A-shaped connector whose pins are repurposed for power/GND/UART. Do not connect this proprietary AC port to a computer or USB charger as though it were normal USB.

For the AC-side cable, the upstream mapping is:

| Physical USB-A pin label | TCL-side purpose inferred from upstream mapping | ESP32-C3 |
|---|---|---|
| GND | module supply | VIN/VCC in the original single-dongle design |
| D+ | signal ground | GND |
| D- | AC TX | GPIO4 RX |
| VBUS | AC RX | GPIO3 TX |

For proxy mode, keep the **factory dongle power and ground wired as in the original TCL connection**, but break the two data conductors and route them through the ESP32:

| Signal path | Connection |
|---|---|
| AC TX -> ESP | AC data-TX conductor -> GPIO4 |
| ESP -> AC RX | GPIO3 -> AC data-RX conductor |
| Factory dongle TX -> ESP | dongle TX conductor -> GPIO5 |
| ESP -> factory dongle RX | GPIO6 -> dongle RX conductor |
| Ground | common between AC, dongle, ESP32 |

### First bench test power recommendation

For the first test, leave the original TCL dongle powered from the AC exactly as stock. Power the ESP32-C3 separately and connect the grounds together. This avoids assuming that the AC dongle supply can safely power both modules at once.

## ESPHome UART configuration

The supplied `tclac-c3-full.yaml` now contains two UARTs:

```yaml
uart:
  - id: tcl_uart
    tx_pin: GPIO3
    rx_pin: GPIO4
    baud_rate: 9600
    data_bits: 8
    parity: EVEN
    stop_bits: 1

  - id: tcl_dongle_uart
    tx_pin: GPIO6
    rx_pin: GPIO5
    baud_rate: 9600
    data_bits: 8
    parity: EVEN
    stop_bits: 1
```

The TCL climate component enables proxy mode with:

```yaml
climate:
  - platform: tclac
    uart_id: tcl_uart
    dongle_uart_id: tcl_dongle_uart
    proxy_mode: true
    proxy_log_packets: false
    proxy_idle_time: 150ms
    proxy_fallback_poll_interval: 15s
```

## Proxy behavior

### Factory dongle -> AC

Every byte arriving on `tcl_dongle_uart` is forwarded immediately to the AC-facing UART. Complete `0xBB` frames are also reconstructed for optional logging.

### AC -> factory dongle

Every byte arriving from the AC is forwarded immediately to the factory dongle **before** the existing TCL status parser processes it. Therefore the stock dongle still receives the AC response while ESPHome sees the same response.

### Home Assistant -> AC

ESPHome-generated control frames are not transmitted in the middle of factory traffic. They are queued until both UART directions have been idle for at least `proxy_idle_time` (default 150 ms).

Only one local frame is queued; if Home Assistant changes settings repeatedly before the bus is free, the newest local control frame replaces the older queued one.

### Polling

When the factory dongle is active, ESPHome does not emit its normal 5-second status poll. The stock dongle's poll and the AC response are enough to update ESPHome.

If the factory dongle is silent for `proxy_fallback_poll_interval` (default 15 seconds), ESPHome sends its own status poll. This lets the ESP32 continue working if the original dongle is removed or offline.

## Packet logging

The existing `Raw Packet Logging` switch now enables both:

- valid AC -> ESP status-frame logs
- factory dongle -> AC frame logs
- ESPHome -> AC injected frame logs

Expected log examples:

```text
[TCL-PROXY] Dongle -> AC [8]: BB 00 01 04 02 01 00 BD
[TCL-PROXY] Dongle -> AC [38]: BB ...
[TCL-PROXY] ESPHome -> AC [38]: BB ...
[TCL] Valid RX[61]: BB ...
```

This is especially useful for reverse-engineering functions available in the official TCL app.

## First test sequence

1. Power everything off.
2. Remove the passive USB splitter.
3. Wire AC UART to GPIO3/GPIO4 as before.
4. Wire the factory dongle UART to GPIO5/GPIO6 through a female connector or breakout.
5. Connect a common ground.
6. For the first test, power the ESP32 separately; leave factory dongle power stock.
7. Flash the updated YAML/component.
8. Boot with `Raw Packet Logging` OFF first.
9. Confirm Home Assistant receives climate status.
10. Confirm the official TCL app sees the AC online.
11. Change temperature once from TCL app; verify AC changes and Home Assistant follows.
12. Change temperature once from Home Assistant; verify AC changes and TCL app later reflects the new state.
13. Enable `Raw Packet Logging` only when capturing/reverse-engineering packets.

## Failure isolation

If Home Assistant works but TCL app does not:

- verify factory dongle TX -> GPIO5
- verify GPIO6 -> factory dongle RX
- verify common ground
- verify the factory dongle still receives its original supply
- enable packet logging and look for `Dongle -> AC` frames

If TCL app works but Home Assistant does not:

- verify GPIO4 receives AC TX
- check for `Valid RX[...]` logs
- verify both UARTs are `9600 8E1`

If neither works:

- power off and re-check data-line direction
- make sure the passive splitter is no longer connecting both TX outputs together
- test the original dongle directly on the AC again to establish a known-good baseline

## Current status

This proxy implementation is a new experimental layer. The original TCL protocol parser/control behavior remains intact, but the dual-UART path needs validation on the physical TAC-PRO12PEC + factory dongle combination before it should be considered production-stable.
