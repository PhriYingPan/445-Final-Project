# Aidan Johnston's Lab Notebook – Desk Learning Aid Device

This notebook documents the comprehensive design, development, testing, and firmware implementation for the Desk Learning Aid Device. It particularly highlights my contributions to the customized PCB and subsystem integration.

---

## 2025-02-01 – Initial Project Direction

**Objectives:**

* Discussed the core project idea and goals with teammates and our TA, Kaiwen Cao.
* Explored engagement mechanisms suitable for classroom environments that avoid excessive screen time.

**Progress:**

* Proposed using a physical interaction system incorporating RFID-based student identification, button-based input, and a rotary encoder or potentiometer scroller.
* Envisioned a backend system supported by Firebase to relay data to a real-time web dashboard accessible to teachers.
* Sketched an initial block diagram representing all subsystems (power, microcontroller, interface, input, and RFID).

---

## 2025-02-12 – Finalize Project

**Objectives:**

* Solidify the overall architecture and prepare for the proposal review.

**Progress:**

* Finalized a refined block diagram detailing subsystem interactions.
* Designed initial device enclosures and mounting plans for discussion with the Machine Shop.
* Scheduled zoom meetings with elementary school teachers to ensure project alignment with classroom needs.
* Began outlining our project proposal for review by course instructors and TAs.

---

## 2025-02-23 – Initial PCB Design Part 1

**Objectives:**

* Begin translating conceptual design into manufacturable electronic schematics.

**Progress:**

* Created a rough draft schematic in KiCad featuring major components: ESP32-S3, push buttons, RFID, voltage regulators, and display interface.
* Assigned preliminary footprints to each component, focusing on pin mapping and layout constraints.

---

## 2025-03-05 – Initial PCB Design Part 2

**Objectives:**

* Convert schematic to a routed PCB design and initiate documentation.

**Progress:**

* Structured the PCB by subsystem (power, input, interface, RFID, microcontroller), allowing for intuitive routing.
* Assigned logical labels, visibility settings, and ground stitching to reduce EMI.
* Helped draft the design documentation section related to PCB layout, component selection, and routing decisions.

---

## 2025-03-13 – Initial PCB Design Part 3

**Objectives:**

* Submit the initial board to PCBWay.

**Progress:**

* Used KiCad's Design Rule Checker to identify and correct errors.
* Submitted and passed PCBWay's manufacturing audit.
* Submitted the design to our TA as apart of the Second Round Orders.

---

## 2025-03-24 – Updated PCB Design Part 1

**Objectives:**

* Evaluate prior board and fix discovered issues.

**Challenges:**

* Incorrect footprint for buck-boost converter (TPS63001).
* Programming circuit for ESP32-S3 not compatible with UART flashing.
* Incorrect footprints for push buttons, inverters, and LCD connector.

**Solutions:**

* Corrected all footprints by referencing datasheets and comparing to KiCad libraries.
* Revised programming method to use USB-to-UART dongle with GND, TX, RX, DTR, RTS lines.
* Rechecked layout and connections for consistency and ease of access.

---

## 2025-03-31 – Updated PCB Design Part 2

**Objectives:**

* Submit improved board design.

**Progress:**

* Rerouted traces and updated board labeling.
* Verified footprint clearances and pad sizes.
* Used KiCad's Design Rule Checker to identify and correct errors.
* Submitted and passed PCBWay's manufacturing audit.
* Submitted the design to our TA as apart of the Third Round Orders.

---

## 2025-04-17 – Hardware Testing

**Objectives:**

* Validate the power subsystem and ESP32 flashing.

**Progress:**

* Measured 3.27V at ESP32 input pin (target: 3.3V).
* Identified error in design: resistor R13 (pull-down) interfered with programming, removed it to enable successful flashing.

---

## 2025-04-25 – Software Testing

**Objectives:**

* Program LCD to verify interface subsystem.

**Progress:**

* Initial attempts to display characters failed.
* Identified reversed SPI pin assignments to LCD: MOSI needed to connect to SID, and MISO to SOD.
* Recognized need for external RST circuit for reliable LCD initialization.

---
## Verification Procedures

**1. DRC Testing**

* All boards verified using KiCad’s DRC.
* Ensured clearance, trace width, and via rules met manufacturing constraints.

**2. Continuity Testing**

* Verified pin-to-pin connections and ground continuity.
* Used multimeter beep function across all critical signal lines.

**3. Power Integrity Test**

* Verified output voltages at ESP32 (3.27V), LCD, and RFID reader.
* Measured battery charge and boost output stability under load.

**4. Boot and Flash Test**

* Helped successfully flash test program using esptool.py.

**5. Input Test**

* Button presses triggered correct GPIO interrupts.
* Potentiometer yielded stable ADC values.

---

## Bibliographic References

1. Espressif Systems. (2023). *ESP32-S3-WROOM-1 & ESP32-S3-WROOM-1U Datasheet*.
2. Firebase. (2024). *Realtime Database Documentation*.
3. ELECTRONIC ASSEMBLY GmbH. (n.d.). *DOGS104E Graphic Display Module*.
4. Texas Instruments. (2008). *TPS63000 High-Efficiency Buck-Boost Converter Datasheet*.
5. TOPPOWER. (2023). *TP4057 Battery Charging and Protection IC Datasheet*.
6. Texas Instruments. (2002). *SN74LS05 Hex Inverter Datasheet*.
7. NXP Semiconductors. (2018). *PN512 NFC Reader IC Datasheet*.
8. Adafruit. (n.d.). *Push Button #492*.
9. Bourns Inc. (2020). *PDB181 Series Potentiometer Datasheet*.
10. IEEE. (2023). *IEEE Code of Ethics*.
11. U8G2 Library Documentation. (2024). *Monochrome Graphics Library for Embedded Devices*.

---

## Notes

* All schematic and layout design in KiCad was completed by Aidan Johnston.
* PCB versions, schematics, and logs available in version-controlled GitHub repository.
