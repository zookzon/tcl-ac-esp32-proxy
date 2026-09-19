# Temperature Step

Adds a Configuration dropdown with two supported target-temperature grids:

- `1.0 °C` (default)
- `0.5 °C`

The selection is persisted across reboot. Changing it performs one safe reboot so Home Assistant receives refreshed Climate traits. The A5 command path also normalizes every requested setpoint to the selected grid, so direct API/service calls cannot bypass the configured step. Range remains 16–31 °C.

No V5 mediator, Beep, Swing, Preset, Action, warm-rejoin, or Energy logic is changed.


## Log publishing

`Temperature Step` is no longer polled every second. Its state is published once after boot and again only when the user changes the dropdown. This removes the repeated `[S][select]` line while preserving the persisted selection and Climate trait behavior.
