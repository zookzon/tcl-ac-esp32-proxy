# TCL machine-operation probe

This build keeps the existing Climate Action logic unchanged.

It adds an `INFO` log tagged `TCL-OP` whenever any of these raw machine-side fields changes:
- Byte 38: compressor-frequency candidate
- Byte 39: current raw (also shown as A)
- Byte 40: compressor-state candidate
- Byte 46: outside-motor candidate

For correlation the same line also includes raw Bytes 35, 36, 37 and 45.

Do not use the existing `Action:` line as evidence. The goal of this build is to identify the actual machine state transition first, then replace Action logic in a later build.
