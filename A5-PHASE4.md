# A5 Takeover Phase 4 — Power ON/OFF + full state capture

Purpose: verify absolute Power command field 0x01 and collect the larger 0C0C state deltas produced by a real OFF→ON transition.

## Added
- `A5 Test Power ON` sends `0A 0A 00 01 01`.
- `A5 Test Power OFF` sends `0A 0A 00 01 00`.
- Every `0C0C` state report is logged in full as `A5-STATE-RAW` before decoding.
- Any `xx 0A` command response is logged. Only `80 0A` is labelled normal ACK; other status values are preserved without guessing.

## Test
Factory dongle remains removed. Cold boot the AC as in previous phases. Wait for valid A5 traffic, then press `A5 Test Power ON` once. Capture 30–60 seconds of log. Do not press other controls during that window. If ON succeeds, press `A5 Test Power OFF` once and capture another 20–30 seconds.
