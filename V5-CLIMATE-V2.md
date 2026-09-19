# V5 Climate v2 — boot-loop fix

Based on V5 Climate v1. The A5 Mediator V5 core is unchanged.

Fixes:
- Prevents Enable Swing Mode / Enable Preset template-switch restore/setup paths from calling safe reboot during boot.
- Adds a non-persistent `feature_switches_armed` boot guard; user actions are armed 5 seconds after setup.
- Persisted `enable_swing_mode` and `enable_preset` globals remain the source of truth across power loss/reboot.
- Climate traits are applied from those persisted globals during boot without rebooting.
- A deliberate user switch change still disables the corresponding AC feature, persists the new capability flag, waits 1.5 s for preferences, and performs one safe reboot so Home Assistant receives updated climate traits.

Expected behavior:
- Power cycle: no reboot loop; saved Enable states survive.
- Enable ON: one reboot; control becomes visible; actual swing/preset remains OFF/NONE until user chooses.
- Enable OFF: corresponding feature is turned OFF/NONE, one reboot, then its climate control is hidden.
