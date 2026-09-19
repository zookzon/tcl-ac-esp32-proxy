# Wiring

This is the wiring used by the tested ESP32-C3 production configuration.

> **TX/RX naming:** the `Signal` column names the signal from the external device's point of view. UART signals cross: device TX connects to ESP32 RX, and device RX connects to ESP32 TX.

## ESP32-C3 Super Mini pin layout

```text
                         USB-C
              dongle  ┌────────────┐  air

       5V ────────────────┐
      GND ──────────────┐ │
                        │ │
       TX ─── GPIO5  ●  │ └───────● 5V   ─── 5V AC
       RX ─── GPIO6  ●  └─────────● GND  ─── GND AC
              GPIO7  ●            ● 3V3
              GPIO8  ●            ● GPIO4 ── AC TX / D-
              GPIO9  ●            ● GPIO3 ── AC RX / D+
             GPIO10  ●            ● GPIO2
             GPIO20  ●            ● GPIO1
             GPIO21  ●            ● GPIO0
                    └──────────────┘
```

The 5V and GND lines are shared between the TCL AC connector, ESP32-C3, and factory dongle. The two UARTs are separate.

## Wiring map

| Side | Device signal | Wire color | ESP32-C3 | Direction |
|---|---|---|---|---|
| Factory dongle | **TX** | White | **GPIO5 (RX)** | Dongle → ESP32 |
| Factory dongle | **RX** | Red/Pink | **GPIO6 (TX)** | ESP32 → Dongle |
| Factory dongle | 5V | Black/Gray | **5V** | Shared 5V rail |
| Factory dongle | GND | Green | **GND** | Shared ground |
| TCL AC | **TX / D-** | White | **GPIO4 (RX)** | AC → ESP32 |
| TCL AC | **RX / D+** | Pink | **GPIO3 (TX)** | ESP32 → AC |
| TCL AC | 5V | Black/Gray | **5V** | Shared 5V rail |
| TCL AC | GND | Green | **GND** | Shared ground |

## UART configuration

### TCL AC UART

```text
ESP32 GPIO3 (TX)  -> AC RX / D+
ESP32 GPIO4 (RX)  <- AC TX / D-
115200 baud, 8N1
```

### Factory dongle UART

```text
ESP32 GPIO6 (TX)  -> Dongle RX
ESP32 GPIO5 (RX)  <- Dongle TX
115200 baud, 8N1
```

## Power topology

```text
AC 5V  ─────┬──── ESP32 5V
            └──── Dongle 5V

AC GND ─────┬──── ESP32 GND
            └──── Dongle GND
```

5V and GND are shared rails. **5V and GND must never be connected to each other.**

## Important safety notes

- Do **not** connect two UART TX outputs together.
- Always cross UART TX/RX: TX → RX and RX ← TX.
- Do **not** treat the TCL connector as a normal USB port just because the connector/wire arrangement may look USB-like.
- Power the AC off before changing wiring.
- Verify the actual pin labels on your ESP32-C3 board revision before soldering.
- This wiring is documented for the tested TCL TAC-PRO12PEC setup; other TCL models or dongle revisions may differ.

## Pins used by this project

| Function | ESP32-C3 pin |
|---|---|
| AC UART TX | GPIO3 |
| AC UART RX | GPIO4 |
| Dongle UART TX | GPIO6 |
| Dongle UART RX | GPIO5 |
| Power | 5V |
| Ground | GND |

GPIO0, GPIO1, GPIO2, GPIO7, GPIO8, GPIO9, GPIO10, GPIO20, GPIO21 and 3V3 are not used by this wiring.
