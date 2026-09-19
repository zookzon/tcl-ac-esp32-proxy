# TAC-PRO12PEC packet map

ทุก index ในเอกสารนี้เป็น **zero-based** และนับ `0xBB` เป็น byte 0

ระดับความมั่นใจ:

- **Confirmed** — ควบคุม/อ่านซ้ำได้ตรงกับสถานะจริง
- **High** — transition log สนับสนุนชัด แต่ยังขาดเครื่องมืออ้างอิงโดยตรง
- **Conditional** — ใช้ได้เฉพาะเงื่อนไขที่ระบุ
- **Experimental** — สมมติฐานสำหรับเก็บหลักฐานเท่านั้น
- **Raw** — เปิดเผย byte โดยไม่ตีความ

## Frame layer

| Field | Mapping | Confidence | Notes |
|---|---|---:|---|
| Header | byte 0 = `0xBB` | Confirmed | parser resynchronizes here |
| Declared payload length | byte 4 | Confirmed | total frame length = byte 4 + 6 |
| RX total lengths | 61, 65, 68 | Supported | 65 observed on TAC-PRO12PEC; 61/68 accepted known variants; other lengths rejected |
| Checksum | final byte | Confirmed | XOR of every preceding byte |
| Poll | `BB 00 01 04 02 01 00 BD` | Confirmed | current status request |
| Control frame | 38 bytes | Confirmed | byte 37 is XOR checksum |

## RX/status map

| Index | Bits/formula | Current interpretation | Confidence and evidence |
|---:|---|---|---|
| 7 | bit 4 | AC power on | Confirmed by state changes |
| 7 | `value & 0x3F` | mode: Auto `0x35`, Cool `0x31`, Fan `0x32`, Dry `0x33`, Heat `0x34` | Confirmed control/status |
| 7 | bit 6 | Eco preset | Confirmed in inherited control path |
| 8 | low nibble + 16 | target °C | Confirmed; 25 °C=`0x89`, 24 °C=`0x88` in observed status |
| 8 | high nibble | fan status: Auto `0x80`, Low `0x90`, Middle `0xC0`, Medium `0xA0`, High `0xD0`, Focus `0xB0` | Working mapping |
| 9 | bit 2 | Comfort preset | Working mapping |
| 10 | bits 5–6 | swing: Off `00`, Horizontal `01`, Vertical `10`, Both `11` | Working mapping |
| 16 | raw | experimental error code | Experimental; observed `0x00` |
| 17–18 | big-endian `raw`; `(raw/374 - 32)/1.8` | room temperature °C | Confirmed behavior; float arithmetic required |
| 19 | bit 0 | Sleep preset | Working mapping |
| 30 | `(byte<<8)/374`, then Fahrenheit-to-Celsius transform | indoor coil temperature | High/Experimental; warmed after compressor stopped |
| 31 | `0xFF` on target unit | unsupported | Confirmed unsupported on tested unit |
| 33 | bit 7 | Quiet fan | Working mapping |
| 35 | raw − 32 | Airmax pipe-out interpretation | Experimental; conflicts with ambient hypothesis |
| 35 | raw − 22 | outdoor ambient candidate | Experimental/leading hypothesis; one 27 °C result matched 26–27 °C thermometer |
| 36 | raw − 32 | Airmax pipe-in interpretation | Experimental |
| 36 | raw − 22 | outdoor exhaust/thermal candidate | Experimental; falls after compressor shutdown |
| 37 | raw − 22 | outdoor condenser/hot-pipe candidate | Experimental; becomes hot and cools slowly |
| 37 | raw | external candidate | Raw only |
| 38 | raw, displayed as Hz candidate | compressor frequency/speed candidate | High/Experimental; follows load and reaches 0 after shutdown |
| 39 | raw / 10.0 | compressor current A | Confirmed/high; FAN_ONLY remains 0.0 A, therefore not whole-unit current |
| 40 | raw | compressor state | Raw/Experimental; `0x8A` running/cooling, `0x80` stopped in observed transition |
| 44 | raw | Airmax fault code | Raw/Experimental; observed 0 |
| 45 | raw 180–255 only | Active Supply Voltage V | Conditional; standby ~147 is a placeholder and becomes unknown |
| 45 | raw | raw byte 45 diagnostic | Raw |
| 46 | raw | outside motor/load state | Raw/Experimental; 1–3 running, 0 stopped in observations |
| 51 | raw | vertical vane position | Raw/Experimental |
| 52 | raw | horizontal vane position | Raw/Experimental |

## TX/control map

The encoder clears all 38 bytes first, then fills these fields.

| Index | Bits/value | Command meaning |
|---:|---|---|
| 0–2 | `BB 00 01` | header |
| 3 | `03` | control command |
| 4 | `20` | control payload length marker |
| 5–6 | `03 01` | inherited constant, exact meaning unknown |
| 7 | bit 2 | power on |
| 7 | bit 5 | Beeper policy |
| 7 | bit 6 | Display policy (only set when mode is not Off) |
| 7 | bit 7 | Eco preset |
| 8 | low bits: Heat `01`, Dry `02`, Cool `03`, Fan `07`, Auto `08` | operating mode |
| 8 | bit 7 | Quiet fan |
| 8 | bit 6 | Diffuse fan |
| 8 | bit 4 | Comfort preset |
| 9 | `31 - target °C` | target temperature command |
| 10 | low fan code: Auto `0`, Low `1`, Medium `3`, Focus `5`, Middle `6`, High `7` | fan level/style |
| 10 | bits 3–5 = `111` | vertical swing enabled |
| 11 | bit 3 | horizontal swing enabled |
| 12 | bit 7 documented upstream as Fahrenheit; current code sends 0 | Celsius |
| 13 | `01` | inherited constant, exact meaning unknown |
| 19 | bit 0 | Sleep preset |
| 32 | bits 3–4 | vertical swing zone: Up-down `01`, Upper `10`, Lower `11` |
| 32 | low 3 bits | vertical fixed: last `0`, max-up `1`, up `2`, center `3`, down `4`, max-down `5` |
| 33 | bits 3–5 | horizontal swing zone: left-right `1`, left `2`, center `3`, right `4` |
| 33 | low 3 bits | horizontal fixed: last `0`, max-left `1`, left `2`, center `3`, right `4`, max-right `5` |
| 37 | XOR bytes 0–36 | checksum |

## Observed transition sequence

One shutdown capture produced this order:

1. Climate mode changed to Off while byte 39 still represented 6.2 A and byte 40
   remained `0x8A`.
2. About 5 seconds later byte 39 became 0.0 A and byte 40 became `0x80`.
3. About 10 seconds after shutdown byte 46 became 0.
4. About 40 seconds after shutdown byte 38 became 0.
5. Byte 35 remained stable while bytes 36/37 cooled.

This is why byte 39 is currently the best proven compressor-running signal,
while bytes 38/40/46 remain supporting diagnostics.

## Rules for changing this map

- Change one AC setting at a time.
- Keep complete checksum-valid frames, not isolated byte values.
- Compare before/during/after transitions and repeat the test.
- Record an independent physical measurement when naming temperature, voltage,
  current, power, or energy.
- Do not promote an Experimental field from one matching sample.
- Do not add a new UART query until its request and response semantics are proven.
