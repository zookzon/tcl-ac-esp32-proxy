# V5 Climate/Telemetry Extension v1

Base: working A5 Mediator V5. The mediator routing/arbitration core is unchanged.

Added:
- Climate swing modes: OFF / VERTICAL / HORIZONTAL / BOTH
  - A5 0x11 vertical louver, 0x0E horizontal louver
  - full-axis sweep uses 0x01; off/parked uses 0x08
- Climate presets: NONE / ECO / SLEEP
  - ECO: A5 field 0x13
  - SLEEP: A5 field 0x22, Standard=1
- Config switches:
  - Enable Swing Mode
  - Enable Preset
  Turning either switch OFF first disables that AC feature, saves the visibility setting,
  then safely reboots so Home Assistant receives the changed climate capabilities.
  Turning ON also resets that feature to OFF/NONE before reboot; it does not select a mode.
- Direct A5 telemetry:
  - Indoor Coil Temperature (0x5C)
  - Input Power (0x64)
  - Compressor Actual (0x65)
  - Compressor Target (0xC0)
  - Air Conditioner Energy integrated from Input Power

Removed from the production YAML because they are legacy BB-derived/estimated values and are
not direct A5 measurements:
- Active Supply Voltage
- Compressor Current Draw
- Compressor Power (Estimated)
- Compressor Energy (Estimated)

Notes:
- Home Assistant climate capabilities are advertised during Native API entity discovery.
  Therefore changing either Enable switch performs a safe reboot so the option truly appears
  or disappears from the climate entity.
- A5 Mediator V5 handling of Factory Dongle traffic (000001 / 0B0B / 0A0A forwarding,
  AC->Dongle mirroring, type23 suppression, and 1515/2525 suppression) was not changed.
