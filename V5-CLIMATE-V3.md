# V5 Climate V3

Base: V5 Climate V2. V5 A5 mediator logic is unchanged.

Changes:
- Preset unchanged: NONE / ECO / SLEEP.
- Added native A5 Home Assistant switches with AC read-back state:
  - Display Light (0x1E)
  - Beep (0x25)
  - Health (0x15)
  - Drying (0x27)
- Sensor presentation order documented/configured as:
  1. Air Conditioner Energy
  2. Input Power
  3. Compressor Target
  4. Compressor Actual
  5. Indoor Temperature
  6. Outdoor Temperature
  7. Indoor Coil Temperature
- Action logic intentionally unchanged in this revision.
- No Voltage entity added: no verified A5 voltage field is currently known.

## Configuration grouping update
- Display Light, Beep, Health, and Drying are marked `entity_category: config` so Home Assistant places them in the Configuration group.
- Indoor Coil Temperature schema is no longer marked `entity_category: diagnostic`; it now appears in the normal Sensors group.

## Entity order adjustment
Requested Home Assistant order:
Sensors: Air Conditioner Energy, Input Power, Compressor Target, Compressor Actual, Indoor Temperature, Outdoor Temperature, Indoor Coil Temperature.
Configuration: Beep, Display Light, Drying, Health, Enable Swing Mode, Enable Preset.
