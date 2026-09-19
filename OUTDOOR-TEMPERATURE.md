# Outdoor Temperature sensor

This build continues directly from `tclac-c3-dual-phase-proxy-test.zip`.

No UART/proxy/dual-phase behavior was changed. The existing byte-35 diagnostic is exposed as the normal Home Assistant sensor `Outdoor Temperature`.

Current decode used by the component:

    Outdoor Temperature = RX byte 35 - 22 °C

`0xFF` is published as unavailable. Bytes 36 and 37 remain disabled diagnostic candidates.

Important: byte 35 is the strongest current outdoor-ambient candidate for TAC-PRO12PEC, but should still be validated against a thermometer near the outdoor-unit air intake under several conditions.
