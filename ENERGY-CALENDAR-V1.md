# Energy Calendar V1

Energy source is the AC-reported A5 field `0x64` (`Input Power`, W). No voltage/current estimate is used.

Entities:
- `Total Energy`: session energy; starts at 0 on every ESP32 boot/restart (`restore: false`).
- `Today Energy`: calendar-day energy; restored across reboot/OTA and reset at 00:00 Asia/Bangkok.
- `Yesterday Energy`: saved value of the last completed day; restored across reboot/OTA.
- `Monthly Energy`: current calendar-month energy; restored across reboot/OTA and reset at 00:00:05 on day 1.

All integration sensors use trapezoidal integration and convert Wh to kWh with a 0.001 multiplier.

The existing V5 mediator, A5 routing, warm rejoin, Action logic, Beep and Swing code are unchanged.
