# TCL TAC-PRO12PEC Dual-Phase Proxy Test

Experimental firmware based on the user's known-working old 9600 proxy code.

## Wiring (unchanged active bridge)
- AC TX -> ESP32 GPIO4 RX
- ESP32 GPIO3 TX -> AC RX
- ESP32 GPIO5 TX -> Factory Dongle RX
- Factory Dongle TX -> ESP32 GPIO6 RX
- Power and GND common

## Runtime
1. First 12 seconds after ESP32 component setup: both UARTs 115200 8N1. Raw AC<->Dongle bytes are bridged. ESPHome local commands/polls are blocked.
2. At 12 seconds: both hardware UARTs are reconfigured to 9600 8E1. RX/parser state is cleared.
3. Normal old-code BB protocol resumes. If factory dongle is quiet, fallback polling resumes after 15 seconds.

## Test
Flash tclac-c3-full.yaml. Keep factory dongle connected. Cut AC mains for 15-20 seconds, restore power, and save the complete first 60-90 seconds of logs.
Look for TCL-DUAL Phase 1/Phase 2, TCL-PROXY, and whether the factory dongle starts transmitting.

NOTE: This is a software experiment. It cannot bridge signals emitted before ESP32/UART initialization.
