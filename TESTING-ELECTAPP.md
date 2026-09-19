# TAC-PRO12PEC clean test diagnostics

## Estimated compressor power and energy

- `Estimated Compressor Power` uses validated active supply voltage ×
  compressor current × power factor.
- Byte 45 is accepted as active supply voltage only from 180 to 255 V. The
  observed standby value of 147 is unavailable and is never integrated.
- The default power factor is `0.95`; edit `estimated_power_factor` under
  `substitutions` after comparison with a true power meter.
- `Estimated Compressor Energy` integrates that estimated power and reports kWh.
- Indoor-fan and standby consumption are not included because byte 39 reports
  zero while FAN_ONLY is active.
- These are estimates, not billing-grade values. No `0x0A` query is transmitted.

This build keeps the existing TCL control path unchanged. It only adds
read-only fields based on ElectApp/TCLAirConditioner commit
`e424477d3e58e2b4b71749049cf413404e0efe4f`.

It also includes read-only diagnostic fields from SkateWarp/ESPHome-Airmax,
after a real TAC-PRO12PEC frame showed plausible values at those positions.

Reference: https://github.com/ElectApp/TCLAirConditioner

## Active entities

- `Experimental Error Code`: frame byte 16, raw decimal value.
- `Experimental Indoor Coil Temperature`: frame byte 30 decoded with the
  ElectApp single-byte temperature formula.
- `Experimental Indoor Coil Raw Byte 30`: unmodified byte 30.
- `Experimental Pipe Out Temperature`: byte 35 minus 32 °C.
- `Experimental Pipe In Temperature`: byte 36 minus 32 °C.
- `Compressor Current`: byte 39 divided by 10 A.
- `Raw Compressor State Byte 40`: unmodified byte 40.
- `Raw Airmax Fault Code Byte 44`: unmodified byte 44.
- `Raw Byte 45 Candidate`: unmodified byte 45, disabled by default. Physical
  measurements and transition logs show that its active-state values track
  supply voltage, while its standby value is not a real mains voltage.
- `Active Supply Voltage`: byte 45 only while it is in the validated active
  range; otherwise unavailable.
- `Raw Outside Motor Byte 46`: unmodified byte 46.
- `Raw Vertical Vane Position Byte 51`: unmodified byte 51.
- `Raw Horizontal Vane Position Byte 52`: unmodified byte 52.

Frame byte indexes are zero-based and include the leading `0xBB` as byte 0.

Byte 31 was removed from the component and YAML after the physical
TAC-PRO12PEC repeatedly returned `0xFF`, confirming that this model does not
provide an outdoor-coil value at that position.

## Safety properties

- A frame is decoded only after its declared length and XOR checksum pass.
- All additions are read-only and do not alter the 38-byte TX control frame.
- No extra UART request is sent.
- Diagnostic sensor states are published only when their value changes.
- Byte 45 is used as supply voltage only in its validated active range.
- Room temperature now uses floating-point division instead of truncating the
  intermediate value.
- Detailed diagnostics and valid packets are logged as one-line hexadecimal
  output only while the `Raw Packet Logging` switch is on. It always starts
  off after boot.

## Test procedure

1. Turn on `Raw Packet Logging` only while collecting a short test capture.
2. Record the experimental entities with the air conditioner in a known state.
3. Turn on COOL mode and record them before and after the compressor starts.
4. Let the unit cool for 10-15 minutes and record them again.
5. Change only one setting at a time and retain the complete `Valid RX[65]`
   log lines.
6. Turn `Raw Packet Logging` off after the capture.

## External unit temperature passive probe

The TCL Home cloud property `externalUnitTemperature` does not provide a
verified UART byte mapping for this model. Two changing bytes are therefore
exposed without conversion as disabled diagnostic entities:

- `Raw External Temperature Candidate Byte 37`
- `Raw External Temperature Candidate Byte 38`

Enable these entities only while testing and compare them with an independent
outdoor thermometer at several times of day. The component does not label
either value as a temperature and does not send any additional UART command.

## Passive Probe v2

A newer third-party fork proposes a second, unverified interpretation of
bytes 35-38. This build exposes it alongside the existing Airmax readings:

- `Experimental Outdoor Ambient Candidate Byte 35`: byte 35 minus 22 °C.
- `Experimental Outdoor Exhaust Candidate Byte 36`: byte 36 minus 22 °C.
- `Experimental Outdoor Condenser Candidate Byte 37`: byte 37 minus 22 °C.
- `Experimental Compressor Frequency Candidate Byte 38`: byte 38 as Hz.

All four entities are disabled by default, read-only, and publish only when
their value changes. They do not alter control packets or add UART requests.
Compare the ambient candidate with a thermometer at the outdoor-unit air
intake over several times of day before assigning a confirmed meaning.
7. Do not use experimental values in automations until their behavior is
confirmed against a physical temperature or electrical measurement.

## Home Assistant humidity source

The ESPHome device exposes a native `Humidity Entity ID` text control. Enter a
Home Assistant humidity sensor such as `sensor.bedroom_humidity`. The selection
is restored after restart and the component subscribes to that entity through
the Native API, without a Home Assistant package, helper, or automation.

Changing the entity ID restarts the ESP32 once so Home Assistant receives the
new subscription. No humidity value is transmitted to the air conditioner; it
is display/state metadata only.

## Room and outdoor temperature entities

`Indoor Room Temperature` publishes the same decoded value used as the climate
entity's current temperature, rounded to one decimal place. It is a separate
read-only sensor and does not add UART traffic.

`Experimental Outdoor Temperature Byte 35` publishes byte 35 minus 22 °C. The
observed 28-29 °C values make it the strongest outdoor-air candidate, while
bytes 36 and 37 rise with compressor load. Keep the experimental qualification
until it has been compared with a thermometer at the outdoor-unit air intake.

## Compressor energy source

`Estimated Compressor Energy` is the cumulative `total_increasing` kWh source
for a user-created Home Assistant Utility Meter Helper. This project does not
create current-month or previous-month entities. The source remains an estimate
that excludes the indoor fan and standby loads.

## Unsupported climate values

The command encoder rejects unsupported climate modes, fan modes, and presets
before sending a UART frame. Supported controls keep their existing mapping.
If an API client supplies an enum that is not advertised by this device, the
component logs a warning and waits for the next real air-conditioner status
update instead of transmitting a partial command.

## Beeper and display switches

`Beeper` and `Display` are native ESPHome configuration switches. Both restore
their last saved choice and default to on for a new installation. A user change
is transmitted immediately only after a checksum-valid status frame has been
received, so startup restoration cannot send a partial climate command.

These are desired write policies rather than confirmed status values: the TCL
status response does not provide a proven readback field for either setting.
