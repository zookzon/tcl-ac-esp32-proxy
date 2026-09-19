# Home Assistant humidity source

The ESPHome device exposes a native text control named `Humidity Entity ID`.
No Home Assistant package, helper, or automation is required.

## Upgrade from the previous package

1. Remove `/config/packages/tcl_ac_humidity.yaml` if it was installed from an
   earlier release.
2. Check the Home Assistant configuration and restart Home Assistant once.
3. Flash the new `tclac-c3-full.yaml` firmware.

Removing the old package avoids duplicate humidity updates from its
`input_select` and automations.

## Select a humidity sensor

1. Open the `tcl-ac` ESPHome device in Home Assistant.
2. Find the **Humidity Entity ID** control.
3. Enter a sensor entity ID, for example `sensor.bedroom_humidity`, and save.
4. The ESP32 automatically restarts once so Home Assistant can register the
   new Native API state subscription.

After reconnecting, the selected sensor value appears as Current Humidity on
the climate entity. The entity ID is saved in flash. Home Assistant and ESP32
restarts automatically restore the selection and current humidity without
changing the text again.

Enter an empty value to disable the external humidity source and clear Current
Humidity. Valid input is blank or a sensor entity ID using lowercase letters,
digits, and underscores (`sensor.example_humidity`). The source state must be a
number from 0 through 100.

The optional `dashboard-card-example.yaml` adds the native text control below
the thermostat card. Replace its entity IDs if Home Assistant generated
different names.

## Compressor energy source

`Estimated Compressor Energy` is the cumulative kWh source exposed by ESPHome.
It has device class `energy` and state class `total_increasing`, and restores
its accumulated value after an ESP32 restart.

Use that entity as the source when creating a Utility Meter Helper in Home
Assistant. No monthly Utility Meter package is included in this project.

The value estimates compressor energy only. It does not include the indoor fan
or standby consumption because the packet current remains zero for those loads.

## Thermostat layout and status

The example card orders its climate controls as follows:

- Row 1: Mode (left), Fan mode (right)
- Row 2: Swing mode (left), Preset (right)

The ESPHome climate component reports actions as follows:

- COOL: action `Cooling` when target is below current; otherwise action `Idle`
- HEAT: action `Heating` when target is above current; otherwise action `Idle`
- DRY: action `Drying` while compressor current is above zero; otherwise `Idle`
- AUTO: `Cooling` or `Heating` from the temperature direction while compressor
  current is above zero; otherwise `Idle`
- FAN_ONLY: `Fan`

The HVAC mode remains Cool, Heat, Dry, or Auto while its action is Idle. The
COOL and HEAT active actions are display rules based on temperature comparison,
not direct compressor-running signals.
