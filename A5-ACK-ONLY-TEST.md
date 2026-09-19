# TAC-PRO12PEC A5 ACK-only takeover test

Purpose: first active-TX test of the A5 protocol using the verified framing/CRC family from codypendant/aciq-minisplit-protocol.

## Safety / wiring

- Factory dongle TX MUST NOT be electrically connected to AC RX while this build is allowed to transmit ACKs.
- This build uses the existing AC-facing UART only: GPIO3 TX -> AC RX, GPIO4 RX <- AC TX, 115200 8N1.
- Keep the known-working TCL harness wiring/voltage arrangement from this project. Do not infer wire colours from the ACiQ repo.

## What this build transmits

Only 12-byte A5 ACK frames, approximately 50 ms after a CRC-valid AC report (type 0x21).
It does NOT send A5 commands, clock replies, RSSI heartbeat, or BB commands/polls.
Legacy climate TX is blocked while `a5_ack_only: true`.

ACK shape:
`A5 01 <link> 23 00 <AC-counter> 00 0C <CRC-H> <CRC-L> 80 <payload-type>`

CRC is CRC-16/XMODEM over the complete frame with bytes 8-9 omitted, matching the referenced repo.

## Expected logs

- `TCL-A5: RX VALID ...` means a complete frame passed CRC.
- `TCL-A5: TX ACK ...` means the ESP32 transmitted an ACK.
- `TCL-A5: CRC FAIL ...` means the received frame did not validate.

The key test is whether the AC's retry/frame rate drops substantially once ACKs begin. Do not use Home Assistant climate controls in this build; they are intentionally blocked.
