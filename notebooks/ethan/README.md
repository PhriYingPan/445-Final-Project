# Ethan Ge's Lab Notebook – Desk Learning Aid Device

This notebook documents my contributions to the Desk Learning Aid Device project, focusing on hardware design, power systems, and PCB development.

---

## 2025-02-10 – Initial Hardware Architecture

**Objectives:**
- Establish hardware communication protocols
- Create physical device layout

**Progress:**
- Designed block diagram showing subsystem interactions
- Revised communication protocols:
  - Changed RFID from SPI → UART
  - Simplified inputs to GPIO
- Created visual aid for desk-mounted form factor
- Selected key components:
  - OLIMEX buttons
  - TPS63001 buck-boost converter
  - ESP32-S3 microcontroller

![Block Diagram](https://github.com/user-attachments/assets/487aa8da-36b6-45d8-8613-9655399f1e17)

---

## 2025-02-17 – Power System Design

**Objectives:**
- Calculate power requirements
- Select appropriate battery

**Calculations:**
```
Total Power Draw = (3.3V * 100mA) / 90% efficiency = 0.36W
Battery Capacity Needed = 0.36W * 8h / 3.7V = 1086mAh
```

**Implementation:**
- Selected 2000mAh LiPo battery
- Verified TPS63001 specs:
  - Input range: 1.8-5.5V
  - Output: 3.3V ±2%
  - Peak efficiency: 96%

**Test Plan:**
| Requirement | Verification Method |
|-------------|---------------------|
| 8h runtime @ 3.0V+ | Continuous operation test |
| Stable 3.3V output | Variable load testing |

---

## 2025-03-01 – PCB Development

**Objectives:**
- Finalize PCB layout
- Address design flaws

**Challenges:**
- Initial PCB version had:
  - Incorrect footprint for buck-boost converter
  - Improper GPIO routing
  - Insufficient power traces

**Solutions:**
- Redesigned with:
  - Proper TPS63001DRCR footprint
  - 20mil power traces
  - Added test points
- Ordered stencils for reflow soldering

![PCB Layout](https://github.com/user-attachments/assets/daa2333a-a7f5-4728-ab04-cc6f4dfd0292)

---

## 2025-03-15 – Hardware Testing

**Objectives:**
- Validate power subsystem
- Test component integration

**Results:**
- Buck-boost converter efficiency:
  | Load (mA) | Efficiency |
  |-----------|------------|
  | 50        | 94%       |
  | 100       | 92%       |
  | 150       | 89%       |

- Battery discharge test:
  - 8h continuous operation
  - Final voltage: 3.27V (meets requirement)
 
![Test Results](https://github.com/user-attachments/assets/5b31bcd8-5508-4f8d-b3f5-f9244525cff5)


**Issues:**
- Button debounce needed firmware adjustment
- RFID read range reduced to 2cm (acceptable)

---

## 2025-03-29 – System Integration

**Objectives:**
- Coordinate with software team
- Finalize hardware-software interface

**Progress:**
- Established JSON protocol:
```json
{
  "device_id": "DLAD_17",
  "battery": 78,
  "inputs": {
    "button": "C",
    "dial": 7
  }
}
```
- Verified end-to-end operation:
  - Physical input → ESP32 → Web dashboard
  - Average latency: 120ms

**Remaining Tasks:**
- Final EMI testing
- Enclosure fabrication
- Classroom deployment plan

---

## Bibliographic References

1. [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
2. [TPS63001 Technical Reference](https://www.ti.com/lit/ds/symlink/tps63001.pdf)
3. [LiPo Battery Characteristics](https://batteryuniversity.com/article/bu-205-types-of-lithium-ion)
4. [PCB Design Guidelines](https://www.analog.com/en/analog-dialogue/articles/pcb-layout-techniques.html)

---

## Notes

- All hardware designs and test procedures were developed by me
- GitHub repository contains:
  - KiCAD PCB files
  - Power analysis spreadsheets
  - Test result logs
