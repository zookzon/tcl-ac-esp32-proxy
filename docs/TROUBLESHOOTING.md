# Troubleshooting

## Home Assistant works, factory TCL app/dongle does not

Check GPIO5/GPIO6 direction, common ground and factory-dongle power. Inspect mediator logs for valid dongle A5 frames. Do not reconnect TX outputs in parallel.

## Factory dongle works, Home Assistant state does not

Check AC TX -> GPIO4, A5 CRC-valid RX, warm-rejoin logs, and that both UARTs are 115200 8N1. Confirm `a5_ack_only: true`.

## State missing after ESP32 restart while AC remains powered

Look for `A5-REJOIN`: INIT should start after boot, stop when a valid `0C 0C` report arrives, then issue one `0B 0B FF FF` query. Do not add arbitrary repeated polling.

## Commands transmit but HA state does not change

The firmware intentionally waits for AC read-back. A TX log alone is not proof the AC accepted a command.

## Excessive logs

Leave RAW A5 logging disabled for normal operation. Uptime and Wi-Fi diagnostics are 60-second updates.

## Energy looks wrong

Input Power is AC-reported A5 data and energy is integrated from it. Compare against an external meter before claiming metering accuracy.
