# Protocol Status Matrix

This file intentionally separates evidence levels.

| Item | Status | Current use |
|---|---|---|
| A5 UART 115200 8N1 on TAC-PRO12PEC | CONFIRMED | Production |
| CRC-16/XMODEM implementation | CONFIRMED | RX validation / TX generation |
| AC type 0x21 report ACK behavior | CONFIRMED | Production service role |
| Clock reply / periodic RSSI obligation | CONFIRMED in project testing | Production |
| `0C 0C` state/delta family | CONFIRMED as state source | Production |
| Factory-style `00 00 01` rejoin INIT | CONFIRMED | Warm state rejoin |
| `0B 0B FF FF` state query | CONFIRMED for project re-sync sequence | One-shot after rejoin |
| Field `0x01` power | CONFIRMED | Climate power |
| Field `0x02` target temperature | CONFIRMED | State/command |
| Field `0x05` fan speed | CONFIRMED/working | Climate fan |
| Field `0x15` Health | CONFIRMED read-back/control | HA switch |
| Field `0x1E` Display Light | CONFIRMED read-back/control | HA switch |
| Field `0x25` Beep | CONFIRMED read-back/control | HA switch |
| Field `0x27` Drying | CONFIRMED read-back/control | HA switch |
| Field `0x64` Input Power | CONFIRMED as AC-reported field in this build | W / energy source |
| Compressor Target / Actual | Working A5 decoded fields | HA sensors |
| Indoor / Outdoor / Indoor Coil temperature | Working A5 decoded fields | HA sensors |
| Unknown A5 fields/classes | UNKNOWN | Preserve/log; do not guess |
| Legacy BB raw candidates | Historical/experimental | Not production authority |

For exact byte construction and parser behavior, current source code wins over historical notes.
