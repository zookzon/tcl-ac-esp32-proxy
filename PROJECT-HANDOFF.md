# เอกสารส่งต่อโปรเจกต์ TCL TAC-PRO12PEC / ESPHome

อัปเดตล่าสุด: 2026-09-05 (Asia/Bangkok)

## 1. เป้าหมายและสถานะปัจจุบัน

โปรเจกต์นี้ควบคุมแอร์ **TCL TAC-PRO12PEC** ด้วย ESP32-C3 ผ่าน UART ของแอร์
และนำเข้า Home Assistant ผ่าน ESPHome Native API โดยพัฒนาต่อยอดจาก
[`thedesp/tclac`](https://github.com/thedesp/tclac/tree/9d9d6ec5c8caebebf5f46cb380b29d9acaab10be)
commit `9d9d6ec5c8caebebf5f46cb380b29d9acaab10be`

หลักการสำคัญของงาน:

- รักษาเส้นทางควบคุม TX เดิมที่ใช้งานจริงได้
- ทำ parser RX ให้ปลอดภัยและรองรับ packet ของ TAC-PRO12PEC
- เพิ่ม diagnostic แบบ passive/read-only โดยไม่เพิ่มคำสั่ง UART ที่ยังไม่ยืนยัน
- แยกค่าที่พิสูจน์แล้วออกจากค่าทดลองอย่างชัดเจน
- ไม่เรียกค่าประมาณกำลัง/พลังงานว่าเป็นมิเตอร์ไฟจริง

ไฟล์หลัก:

- `tclac-c3-full.yaml` — configuration พร้อมใช้งาน
- `components/tclac/climate.py` — ESPHome schema/code generation
- `components/tclac/tclac.h` — constants, class state และ sensor pointers
- `components/tclac/tclac.cpp` — UART parser, decoder, encoder และ HA state
- `PACKET-MAP.md` — packet map พร้อมระดับความมั่นใจ
- `TEST-PLAN.md` — ขั้นตอนเก็บหลักฐานและ reverse engineering ต่อ

## 2. ฮาร์ดแวร์และการสื่อสาร

- MCU: ESP32-C3
- ESPHome board: `esp32-c3-devkitm-1`
- TX จาก ESP32: GPIO3
- RX เข้า ESP32: GPIO4
- UART: 9600 baud, 8E1 (8 data bits, even parity, 1 stop bit)
- Frame header: `0xBB`
- Poll frame ที่ใช้อยู่: `BB 00 01 04 02 01 00 BD`
- รอบ polling: ตาม update interval ของ climate component (ปัจจุบันประมาณ 5 วินาที)

**คำเตือน:** หัวต่อของแอร์มีรูปร่างคล้าย USB แต่ไม่ใช่ USB ห้ามเสียบกับ PC
หรือ USB charger และไม่ควรเปลี่ยนวงจร level/power ของชุดที่ใช้งานได้อยู่แล้ว
โดยไม่มีการวัดทางไฟฟ้า

## 3. การติดตั้งและ compile

ต้องสร้าง `secrets.yaml` เอง โดยใช้ชื่อ key ต่อไปนี้เท่านั้น:

```yaml
wifi_ssid: "..."
wifi_password: "..."
fallback_ap_password: "..."
api_encryption_key: "..."
ota_password: "..."
```

คำสั่งตรวจและ compile:

```bash
esphome config tclac-c3-full.yaml
esphome compile tclac-c3-full.yaml
```

ผลยืนยันล่าสุดบน ESPHome 2026.7.4:

- config validation ผ่าน
- full compile ผ่าน ไม่มี warning/error จาก `tclac.cpp`
- RAM 33.2% (106,714 / 321,296 bytes)
- Flash 56.0% (1,028,230 / 1,835,008 bytes)
- app binary `0xfb1f0` bytes และเหลือพื้นที่ app partition 44%

## 4. ฟังก์ชันที่ใช้งานอยู่

### Climate

- Mode: Off, Auto, Cool, Heat, Dry, Fan only
- Fan: Auto, Quiet, Low, Middle, Medium, High, Focus, Diffuse
- Swing: Off, Vertical, Horizontal, Both
- Preset: None, Eco, Sleep, Comfort
- Target temperature: 16–31 °C, step 1 °C
- Current temperature: 0.1 °C
- Current humidity: รับจาก HA sensor ที่ผู้ใช้เลือก

### Entity เพิ่มเติม

- `Indoor Room Temperature` — ค่าเดียวกับ Climate current temperature
- `Experimental Outdoor Temperature Byte 35` — `RX[35] - 22`; ยังทดลอง
- `Compressor Current` — `RX[39] / 10` A
- `Active Supply Voltage` — ใช้ `RX[45]` เฉพาะช่วง 180–255 V
- `Estimated Compressor Power` — V × A × PF (ค่า PF เริ่มต้น 0.95)
- `Estimated Compressor Energy` — integration เป็น kWh, restore หลัง reboot
- diagnostic/raw sensors ตาม `PACKET-MAP.md`
- Wi-Fi Signal, Uptime, IP Address, SSID และ ESPHome Version

### Beeper และ Display

- เป็น template switches ประเภท config
- จำค่าหลัง reboot และค่าเริ่มต้นสำหรับการติดตั้งใหม่คือ ON
- เมื่อผู้ใช้เปลี่ยนค่า จะส่งคำสั่งหลังจากได้รับ status frame ที่ checksum ถูกต้องแล้ว
- ระหว่าง boot จะบันทึก policy อย่างเดียว ไม่ส่ง partial command
- เป็น **desired/write policy** ไม่ใช่สถานะที่อ่านยืนยันกลับจากแอร์
- ไม่เปิด `Force config` ให้ผู้ใช้
- หมายเหตุจาก upstream source ระบุว่าการปิด Display อาจทำให้บางรุ่นเปลี่ยนเป็น
  Auto; ยังไม่มีหลักฐานยืนยันแยกสำหรับ TAC-PRO12PEC จึงต้องทดสอบก่อนพึ่งพา

## 5. Humidity Entity ID

อุปกรณ์มี text entity ชื่อ `Humidity Entity ID` ผู้ใช้กรอก entity ID เช่น
`sensor.bedroom_humidity`:

- pattern ที่รับ: ช่องว่าง หรือ `sensor.[a-z0-9_]+`
- ค่าเก็บใน flash
- เมื่อแก้ entity ID จะรอ 750 ms แล้ว safe reboot หนึ่งครั้ง
- component subscribe ค่า HA ผ่าน Native API โดยตรง
- รับเฉพาะตัวเลข finite ช่วง 0–100%
- ถ้าค่าว่าง/ผิด/นอกช่วง จะล้าง Current Humidity เป็น unknown
- subscription เก่าที่อาจยังอยู่จะถูก ignore ด้วยการเทียบ selected entity ID
- humidity ใช้แสดงใน Climate เท่านั้น ไม่ส่งไปควบคุมแอร์

ไม่ต้องใช้ HA package/helper/automation รุ่นเก่า หากเคยติดตั้ง
`/config/packages/tcl_ac_humidity.yaml` ให้ลบออกเพื่อป้องกัน duplicate writer

## 6. Parser และความปลอดภัยที่แก้จาก upstream

### RX parser

- เปลี่ยนจากการอ่าน frame แบบ blocking เป็น non-blocking accumulator
- sync ที่ header `0xBB`
- อ่าน declared length จาก `RX[4]`; total length = `RX[4] + 6`
- ยอมรับเฉพาะ total length 61, 65 หรือ 68 bytes; เครื่องเป้าหมายยืนยัน 65 bytes
  ส่วน 61/68 เป็น known variants ที่ parser รองรับ
- buffer RX ขนาด 68 bytes พร้อม overflow guard
- timeout 250 ms สำหรับ partial frame
- decode เฉพาะ frame ที่ XOR checksum ถูกต้อง
- packet log เป็น hex หนึ่งบรรทัด อ่านง่าย

### TX encoder

- เคลียร์ `dataTX[38]` ด้วย `memset` ทุกครั้งก่อนประกอบคำสั่ง
- ป้องกัน byte ที่ไม่ได้กำหนดค้างจาก RAM/stale command
- enum Mode/Fan/Preset ที่ component ไม่รองรับถูก reject ก่อน `sendData()`
- log warning แล้วไม่ส่ง partial/invalid UART frame
- แก้ compiler warnings สำหรับ `HEAT_COOL`, `FAN_ON/OFF`, `HOME`, `AWAY`,
  `BOOST`, `ACTIVITY`
- Beeper/Display ขณะเริ่มจากสถานะ OFF มี fallback preset None และ fan Auto
  เมื่อ optional state ยังไม่มีค่า

### Sensor/API traffic

- diagnostic sensor publish เฉพาะเมื่อค่าเปลี่ยน
- `Raw Packet Logging` เริ่ม OFF ทุก boot และควรเปิดเฉพาะช่วงเก็บ log
- ไม่เพิ่มคำสั่ง `0x0A` หรือ cloud-property polling ที่ยังไม่ยืนยัน

## 7. ปัญหาที่พบและวิธีแก้

### 7.1 RX packet ยาวกว่า upstream buffer

**อาการ:** upstream คาด 61 bytes แต่เครื่องนี้ส่ง 65-byte status frame และมีข้อมูล
เพิ่มเติมท้าย frame

**แก้:** buffer 68 bytes, length validation และรองรับ 61/65/68 bytes

### 7.2 การอ่าน UART แบบ blocking/partial frame

**ความเสี่ยง:** loop ค้าง, frame ขาด, overflow หรือ decode packet ที่ยังมาไม่ครบ

**แก้:** non-blocking accumulator, timeout, header sync, declared-length check,
overflow guard และ checksum gate

### 7.3 TX buffer ไม่ได้ initialize ครบ

**ความเสี่ยง:** byte 29 และตำแหน่งอื่นอาจมีค่า RAM เก่าปะปน

**แก้:** `std::memset(dataTX, 0, sizeof(dataTX))` ก่อนประกอบทุก command

### 7.4 Room temperature สูญเสียทศนิยม

**สาเหตุ:** intermediate arithmetic แบบ integer

**แก้:** รวม bytes 17–18 เป็น `uint16_t`, คำนวณ float แล้ว round 0.1 °C

### 7.5 byte 31 ถูกตีความเป็น sensor ทั้งที่เครื่องส่ง `0xFF`

**แก้:** ลบการใช้งาน byte 31 สำหรับรุ่นนี้และถือว่า unsupported

### 7.6 byte 45 เคยถูกเข้าใจว่าเป็น voltage ตลอดเวลา

**อาการ:** ตอน standby ได้ 147 ซึ่งไม่ใช่แรงดันไฟบ้านจริง

**หลักฐานใหม่:** ตอน compressor เริ่มทำงาน byte 45 เปลี่ยนเป็นประมาณ 228–230
และลดตาม load อย่างสมเหตุผล; ตอน OFF กลับเป็น placeholder ราว 147

**แก้:** publish `Active Supply Voltage` เฉพาะ raw 180–255; นอกช่วงเป็น unknown
และไม่ integrate ช่วงนั้นเข้า energy

### 7.7 Power/Energy ถูกเข้าใจว่าเป็นไฟรวมทั้งเครื่อง

**หลักฐาน:** FAN_ONLY ยังมีพัดลมในห้องทำงาน แต่ byte 39 = 0.0 A

**แก้:** ใช้ชื่อ `Estimated Compressor Power/Energy` และระบุชัดว่าไม่รวม indoor
fan กับ standby; หากต้องการค่าจริงทั้งเครื่องต้องใช้ external energy meter

### 7.8 Climate action แสดง Off ทั้งที่ mode ยังเปิด

**แก้:** ใช้ `Idle` เมื่อ Cool/Heat/Dry/Auto ยังเปิดแต่ไม่เข้าเงื่อนไข active;
ใช้ `Off` เฉพาะ mode OFF

### 7.9 HA humidity package มี restart race

**อาการ:** helper จำตัวเลือกได้ แต่ humidity ไม่ republish จนกว่าจะเปลี่ยนตัวเลือก

**แก้:** ย้ายมาเป็น native ESPHome text + dynamic Native API subscription และ
reboot หลังเปลี่ยน ID

### 7.10 Compiler enum warnings

**อาการ:** switch ไม่ครอบคลุม enum ใหม่ของ ESPHome

**แก้:** default guard ปฏิเสธค่าที่ไม่รองรับก่อนส่ง UART; การควบคุมค่าปกติเดิม
ไม่เปลี่ยน

## 8. HVAC action ปัจจุบันและข้อจำกัด

ปัจจุบัน:

- OFF → Off
- FAN_ONLY → Fan
- COOL: target < current → Cooling; นอกนั้น Idle
- HEAT: target > current → Heating; นอกนั้น Idle
- DRY: compressor current > 0 → Drying; นอกนั้น Idle
- AUTO: current/target direction + compressor current > 0 → Cooling/Heating;
  นอกนั้น Idle

ข้อจำกัด: COOL/HEAT ยังเป็น heuristic จากอุณหภูมิ ไม่ได้ยืนยันว่า compressor
ทำงานจริง จึงอาจแสดง Cooling/Heating ก่อน compressor เริ่ม ส่วน AUTO ยังไม่มี byte
ที่พิสูจน์แล้วว่าบอกทิศทาง cooling/heating

แนวทางที่ตกลงไว้แต่ **ยังไม่ทำ**:

- COOL/HEAT/DRY ใช้ Mode + byte 39 > 0 A เป็น active evidence
- AUTO ต้องหา source byte/flag บอก cooling vs heating ก่อน
- เก็บ log AUTO ในช่วง Cooling, Heating และ Idle แล้วทำ packet diff

## 9. หลักฐาน packet สำคัญ

- Target 25 °C แสดง status `RX[8] = 0x89`; เปลี่ยนเป็น 24 °C แล้ว status เป็น
  `0x88` ภายในประมาณ 317 ms และคงอยู่ในการ poll ถัดไป
- ขณะ cooling พบ byte 39 raw 22/38/44/62/78 สอดคล้อง 2.2/3.8/4.4/6.2/7.8 A
- หลังสั่ง OFF: mode เป็น OFF ก่อน แต่ current/state ยัง 6.2 A/`0x8A` ชั่วคราว;
  ราว 5 วินาที current เป็น 0 และ state `0x80`; ราว 10 วินาที byte 46 เป็น 0;
  ราว 40 วินาที byte 38 เป็น 0
- byte 30 อุ่นจากประมาณ 5.8 °C ไป 17.2 °C หลังปิด compressor สนับสนุนว่าเป็น
  indoor coil temperature
- byte 35 raw 49 คงที่ระหว่าง shutdown และ `49-22 = 27 °C`; เทอร์โมมิเตอร์
  ภายนอกวัด 26–27 °C ในตัวอย่างเดียว
- byte 36 ลดจาก raw 58 ไป 47–48 หลัง shutdown; เป็น thermal/pipe candidate
- byte 37 ลดช้าจาก raw 82 ไป 76; เป็น hot condenser/pipe candidate มากกว่า ambient
- byte 38 เปลี่ยนตาม compressor และสุดท้ายเป็น 0; สนับสนุน frequency/speed
- byte 40 `0x8A` พบขณะ cooling/running และ `0x80` เมื่อ stopped/standby
- byte 46 พบ 1–3 ตาม load และ 0 เมื่อหยุด แต่ความหมายแต่ละระดับยังไม่ decode

หลักฐานทั้งหมดเป็นของเครื่อง TAC-PRO12PEC ตัวที่ทดสอบ ไม่ควรเหมารวมกับ TCL
ทุกรุ่นโดยไม่ตรวจ packet ของเครื่องเป้าหมาย

## 10. ค่าที่ยืนยันและค่าทดลอง

### ใช้งานเป็นชื่อจริงได้

- Power state, Mode, Target Temperature
- Indoor Room Temperature / Climate current temperature
- Fan Mode, Swing Mode, Preset (เส้นทางควบคุมที่ใช้จริง)
- Compressor Current โดยต้องเข้าใจว่าไม่ใช่กระแสรวม
- HA Current Humidity จาก sensor ที่เลือก
- Wi-Fi/uptime/network diagnostics

### ใช้งานได้แต่ต้องมีคำกำกับ

- Active Supply Voltage — เฉพาะ 180–255 V
- Estimated Compressor Power
- Estimated Compressor Energy

### ต้องคง Experimental/Raw

- byte 16 error interpretation
- byte 30 indoor coil formula แม้พฤติกรรมสนับสนุนสูง
- byte 35/36/37 ทุกชื่อ temperature candidate
- byte 38 compressor frequency candidate แม้พฤติกรรมสนับสนุนสูง
- byte 40 compressor state bit meanings
- byte 44 fault code
- byte 45 raw
- byte 46 outside motor levels
- byte 51/52 vane positions

## 11. Energy และ Utility Meter

สูตรปัจจุบัน:

```text
Estimated Compressor Power (W) = Active Voltage × Compressor Current × 0.95
Estimated Compressor Energy (kWh) = integral(Power over hours) × 0.001
```

- update ทุก 5 วินาที
- integration method: trapezoid
- energy `restore: true`
- `device_class: energy`
- `state_class: total_increasing`
- accuracy 0.001 kWh
- ถ้ากระแส <= 0 ให้ power 0
- ถ้ากระแส > 0 แต่ voltage invalid ให้ visible power unknown และ integration source 0

ผู้ใช้จะสร้าง Home Assistant Utility Meter Helper เอง:

- source: `Estimated Compressor Energy`
- cycle: Monthly
- Delta values: Off
- Net consumption: Off
- Source periodically resets: Off

ห้ามเพิ่ม Current/Previous Month entity หรือ HA monthly package กลับมา เว้นแต่ผู้ใช้
เปลี่ยนความต้องการ

## 12. สิ่งที่ตั้งใจไม่ทำ

- ไม่จัดลำดับ Fan Mode ใหม่ เพราะ standard ESPHome fan modes ถูกส่งตาม enum/bitmask
  order; การบังคับลำดับต้องเปลี่ยนเป็น custom modes และ mapping ใหม่
- ไม่เปิด Force config ให้ผู้ใช้
- ไม่เปิด Display on module เพราะต้องทราบ GPIO LED ของ hardware
- ยังไม่เปิด fixed vane/swing-zone controls เพราะไม่ได้ทดสอบทุกตำแหน่ง
- ไม่ใช้ Today/Yesterday Energy, Work Time, Is Online หรือคำสั่ง poll `0x0A`
- ไม่เรียก byte 35 ว่า `Outdoor Temperature` แบบยืนยันแล้ว

## 13. งานถัดไปที่แนะนำ

1. พิสูจน์ byte 35 กับเทอร์โมมิเตอร์ที่ช่องลมเข้า outdoor unit หลายอุณหภูมิ
   (กลางวัน/กลางคืน, OFF, และ compressor ทำงาน 15–30 นาที)
2. เก็บ AUTO mode ทั้ง cooling/heating/idle เพื่อหา direction flag
3. เทียบ byte 38 กับเครื่องมือหรือ official diagnostic ถ้าเข้าถึงได้
4. ทำ controlled test ของ byte 40 และ byte 46 ทีละ transition
5. ทดสอบ byte 51/52 โดยเปลี่ยน vane ทีละตำแหน่งและบันทึก packet
6. เทียบ Estimated Power กับมิเตอร์จริงหลาย load ก่อนปรับ PF
7. ทดสอบ Beeper/Display จาก OFF และทุก mode พร้อมตรวจว่าปิด Display มี side effect
8. พิจารณาแก้ optional `.value()` ใน `control()` ให้มี fallback ทุกจุดเหมือน
   `takeControl()` เพื่อ hardening เพิ่มเติม หากพบ call ก่อน status แรก

## 14. Release history ที่ควรรู้

- `tclac-c3-clean-test-source.zip` — parser/diagnostic รุ่นต้น
- `tclac-c3-external-temp-passive-probe-v2-source.zip` — bytes 35–38 probes
- `tclac-c3-power-energy-estimate-source.zip` — **obsolete** เพราะเคยใช้ byte 45
  เป็น voltage โดยไม่มี active-range guard
- `tclac-c3-active-voltage-humidity-auto-source.zip` — active voltage + HA package
- `tclac-c3-native-humidity-text-all-modes-source.zip` — native humidity text
- `tclac-c3-safe-enum-guard-source.zip` — enum guards
- `tclac-c3-beeper-display-source.zip` — source ล่าสุดก่อนเอกสาร handoff

ให้ใช้ไฟล์ใน handoff ZIP ล่าสุดเท่านั้น และห้ามย้อนใช้ power-energy archive ที่ระบุ
obsolete

## 15. แหล่งอ้างอิง

- Upstream base: <https://github.com/thedesp/tclac/tree/9d9d6ec5c8caebebf5f46cb380b29d9acaab10be>
- ElectApp diagnostics: <https://github.com/ElectApp/TCLAirConditioner>
- Airmax cross-reference: <https://github.com/SkateWarp/ESPHome-Airmax>
- Protocol notes (ใช้เทียบเท่านั้น ไม่ถือว่าเหมือนทุกโมเดล):
  <https://github.com/Kannix2005/esphome-tcl-ac/blob/main/PROTOCOL.md>

เมื่อข้อมูลจาก repo อื่นขัดกับ log ของ TAC-PRO12PEC ให้ยึด packet ที่วัดจากเครื่องจริง
และคงชื่อ Experimental จนมีหลักฐานอิสระยืนยัน

## 16. วิธีเทียบกับ upstream

```bash
git clone https://github.com/thedesp/tclac.git upstream-tclac
git -C upstream-tclac checkout 9d9d6ec5c8caebebf5f46cb380b29d9acaab10be
diff -ru upstream-tclac/components/tclac components/tclac
```

ความแตกต่างจำนวนมากเป็น parser hardening, ESPHome API compatibility และ diagnostic
fields จึงไม่ควรแทนไฟล์ปัจจุบันด้วย upstream ทั้งโฟลเดอร์โดยตรง ให้ cherry-pick หรือ
port การเปลี่ยนแปลงทีละส่วนแล้วทำ regression checklist ทุกครั้ง
