# TCL TAC-PRO12PEC — A5 production takeover

This build promotes the proven A5 path to the Home Assistant `Air Conditioner` climate entity.

- 115200 8N1 A5 framing
- ACK, clock reply and periodic RSSI module obligations retained
- Home Assistant mode: OFF / AUTO / COOL / HEAT / DRY / FAN_ONLY
- Home Assistant target temperature sends the verified dual-record setpoint command
- Fan controls map to protocol speeds 0..7; auto sends both 0x73=1 and 0x05=0
- Climate state is published only from AC 0C0C reports, not optimistically from commands
- Indoor and outdoor temperatures are fed from A5 state
- Legacy BB polling is disabled in A5 mode

IMPORTANT: stock factory dongle TX must remain disconnected while this build owns TX.
