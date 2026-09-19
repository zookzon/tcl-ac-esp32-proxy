# Test and reverse-engineering plan

เอกสารนี้กำหนดวิธีเก็บหลักฐานให้คนรับช่วงสามารถทำซ้ำและเปรียบเทียบ packet ได้

## 1. เตรียมระบบ

1. ใช้ source ล่าสุดทั้งโฟลเดอร์ ห้ามคัดลอกเฉพาะ YAML แล้วอ้าง component เก่า
2. สร้าง `secrets.yaml` ในเครื่องตนเองและห้ามแนบเข้า issue/log/archive
3. รัน `esphome config tclac-c3-full.yaml`
4. รัน `esphome compile tclac-c3-full.yaml`
5. แฟลชและตรวจว่า climate control พื้นฐานตอบสนองก่อนเปิด diagnostics
6. เปิด disabled diagnostic entities เฉพาะค่าที่กำลังทดสอบ

## 2. วิธีเก็บ log

1. เปิด switch `Raw Packet Logging`
2. รอ baseline อย่างน้อย 3 status frames
3. เปลี่ยน **หนึ่งค่าเท่านั้น**
4. เก็บ `Valid RX[length]: ...` เต็มบรรทัด พร้อม timestamp
5. เก็บ Diagnostics line ในช่วงเดียวกัน
6. รอให้ค่าคงตัวหรือผ่าน transition ที่ต้องการ
7. ปิด `Raw Packet Logging` ทันทีหลังจบ

ข้อมูลประกอบที่ต้องจด:

- วันที่/เวลาและ timezone
- โหมด, target, fan, swing, preset
- แอร์ ON/OFF และเวลาตั้งแต่สั่ง
- อุณหภูมิห้อง/ภายนอกจากเครื่องมือแยก
- power/current/voltage จาก external meter ถ้ามี
- ตำแหน่งที่วาง sensor และสภาพ compressor/fans
- firmware archive/checksum ที่ใช้

## 3. External temperature / bytes 35–37

วางเทอร์โมมิเตอร์ที่ช่องลม **เข้า** outdoor unit ไม่ใช่ลมร้อนออก แล้วเก็บ:

- กลางคืนและกลางวันซึ่งอุณหภูมิต่างกันชัด
- แอร์ OFF จน coil เย็น
- ก่อน compressor เริ่ม
- หลัง compressor ทำงานต่อเนื่อง 5, 15 และ 30 นาที
- หลัง OFF ที่ 5, 10, 30 และ 60 นาที

เกณฑ์พิจารณา byte 35 เป็น ambient:

- `byte35 - 22` ต้องตามเครื่องมืออิสระซ้ำหลายช่วงอุณหภูมิ
- error โดยทั่วไปควรอยู่ราว ±2 °C หรือน้อยกว่า
- ต้องไม่เพิ่มตามความร้อนของ condenser อย่างมีนัยสำคัญ
- ถ้ายืนยันครบจึงค่อยเปลี่ยนชื่อออกจาก Experimental

## 4. HVAC action / AUTO direction

เก็บอย่างน้อยสาม state ของ AUTO:

- compressor idle
- AUTO กำลัง cooling
- AUTO กำลัง heating (ถ้าเครื่อง/สภาพอากาศรองรับ)

สำหรับแต่ละ state ให้เก็บ baseline 3 frames และ active 6–12 frames แล้วทำ byte diff
โดยจับตา bytes 38, 39, 40, 44–46 และ bytes อื่นที่เปลี่ยนเหมือนกันทุกครั้ง

ห้ามสรุป direction จาก target/current อย่างเดียว เป้าหมายคือหา flag ที่ packet ส่งมา
จากสถานะจริง หากยังหาไม่ได้ให้ AUTO action คงเป็น heuristic/Idle ตามเดิม

## 5. Compressor diagnostics

ทดสอบ COOL อย่างน้อย:

- OFF steady state
- ON แต่ compressor delay
- compressor low, medium, high load
- หลังถึง target และ compressor ลด/หยุด
- shutdown cooldown

เทียบ:

- byte 38 candidate frequency
- byte 39 compressor current
- byte 40 raw state
- byte 45 active voltage
- byte 46 outside motor
- external clamp/power meter ถ้ามี

## 6. Beeper และ Display

ทดสอบทีละ switch ใน OFF, COOL, FAN_ONLY และหลัง reboot:

- switch จำค่าได้
- เปลี่ยนแล้วคำสั่งอื่น (mode/target/fan/swing/preset) ไม่เปลี่ยน
- Beeper ตรงกับเสียงที่คาด
- Display ตรงกับหน้าจอแอร์
- ตรวจเป็นพิเศษว่าปิด Display ทำให้ mode เปลี่ยนเป็น Auto หรือไม่
- เนื่องจากไม่มี readback ที่ยืนยัน อย่าใช้ switch state เป็นหลักฐานว่า physical state
  เปลี่ยนสำเร็จโดยไม่ตรวจเครื่องจริง

## 7. Vane positions

เริ่มจาก swing Off แล้วเปลี่ยน fixed position ทีละขั้น เก็บ packet ก่อนและหลังทุกตำแหน่ง
จากนั้นทำซ้ำสำหรับ vertical และ horizontal แยกกัน ห้ามเปิดทั้งสองแกนพร้อมกันในการหา map
รอบแรก

## 8. Power-factor calibration

ใช้ external whole-unit meter และจดพร้อม packet อย่างน้อย 5 load levels ห้าม calibrate
จากจุดเดียว เพราะ inverter AC มี PF/efficiency เปลี่ยนตาม load

ค่าจากโปรเจกต์ยังไม่รวม indoor fan และ standby ต่อให้ปรับ PF แล้ว จึงต้องคงคำว่า
Estimated Compressor เสมอ

## 9. รูปแบบรายงานผล

แนบข้อมูลนี้ทุกครั้ง:

```text
Model:
Firmware ZIP SHA-256:
ESPHome version:
Test time/timezone:
Known AC state:
Physical measurement:
Action performed:
Frames before:
Frames during:
Frames after:
Changed byte indexes:
Repeated test count:
Conclusion:
Confidence: Confirmed / High / Conditional / Experimental / Raw
```

## 10. Regression checklist ก่อนส่ง release

- Config validation ผ่าน
- Full compile ผ่านและค้น log ไม่พบ `warning:` หรือ `error:` จาก component
- Mode ทุกตัวควบคุมได้
- target 16/24/31 °C ทำงาน
- fan/swing/preset อย่างน้อยหนึ่งรอบทุกตัวเลือก
- OFF ไม่ถูกแสดงเป็น Idle; active mode ที่พักไม่ถูกแสดงเป็น Off
- Humidity ID blank/valid/invalid ทำงานตามกำหนด
- Beeper/Display จาก OFF และ ON ไม่ส่ง partial command
- Raw Packet Logging เริ่ม OFF หลัง reboot
- Energy เพิ่มเฉพาะช่วง estimate power มากกว่า 0
- ZIP ไม่มี `secrets.yaml`, `.esphome`, `build`, `__pycache__`
- บันทึก SHA-256 ของ ZIP
