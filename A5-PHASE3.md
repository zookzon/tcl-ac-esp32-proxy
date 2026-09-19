# A5 Takeover Phase 3 — State decoder

Adds a conservative 0C0C state/delta decoder based on the verified codypendant A5 field map.

Logs cached state under `A5-STATE`: Power, Mode, Setpoint, Room, Outdoor, Fan, Compressor Target/Actual, Input Power.

Adds absolute test buttons for 24C, 25C, and 26C. Factory dongle must remain disconnected while A5 TX is active.

Important: A5 state reports are deltas. A `?` means that field has not yet appeared since this ESP boot; it is not assumed from an outgoing command.
