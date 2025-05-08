# Conan Pan's Lab Notebook – Desk Learning Aid Device

This notebook documents the design, development, and testing efforts for the Desk Learning Aid Device, specifically focusing on the web application and firmware components I was responsible for.

---

## 2025-02-01 – Initial Project Direction

**Objectives:**
- Discussed core project idea with teammates and TA.
- Brainstormed solutions for capturing student engagement without screens.

**Progress:**
- Proposed idea of combining physical desk input (RFID, buttons, scroller) with a teacher-facing web dashboard.
- Identified potential microcontroller (ESP32-S3) and real-time database (Firebase).
- Sketched initial system block diagram.

---

## 2025-02-14 – WiFi Communication Plan

**Objectives:**
- Finalize communication protocol between ESP32 and Web App.

**Progress:**
- Decided to send data from ESP32 → Web App via Wifi, with ESP32 acting as Wifi server.
- Drafted JSON data schema:
```json
{
  "timestamp": 1712159000000,
  "rfid": "04A3C2",
  "emotion": 7,
  "selectedAnswer": "B",
  "question": "Which fraction is smallest?",
  "subject": "Math",
  "correctAnswer": "C",
  "isCorrect": false
}
```

---

## 2025-03-05 – Breadboard Prototype

**Objectives:**
- Demonstrate basic functionality with ESP32-S3 on breadboard.

**Progress:**
- Wired 5 push buttons, 1 rotary scroller, RFID reader.
- Developed C firmware (`hello_world_main.c`) to:
  - Debounce button presses.
  - Sample potentiometer input via ADC1_CH8.
  - Send responses to Firebase via HTTPS.
- Verified WiFi round-trip latency under 100ms and analog scaling of emotion (1–10).

---

## 2025-03-13 – Dashboard Development

**Objectives:**
- Build interactive frontend dashboard for teachers.

**Progress:**
- Implemented `StudentDashboard.jsx` in React:
  - Displays correctness history, emotion bar, and raise-hand notifications.
  - Allows teacher to push prompts to Firebase.
  - Visualizes subject performance using Chart.js.
- Integrated Firebase Realtime Database and onValue listeners.

<img width="242" alt="Screenshot 2025-05-01 at 3 58 56 PM" src="https://github.com/user-attachments/assets/02a5c67c-e478-4602-b1a5-a3d076fe4876" />


<img width="221" alt="Screenshot 2025-05-01 at 3 59 00 PM" src="https://github.com/user-attachments/assets/1d1b1cf3-f279-4cc3-9624-42b7591bee39" />


<img width="225" alt="Screenshot 2025-05-01 at 4 01 00 PM" src="https://github.com/user-attachments/assets/b395e531-ea42-40e3-8f75-d1a915d00ee8" />




---

## 2025-03-24 – Tolerance and Signal Analysis

**Objectives:**
- Evaluate WiFi reliability in classroom conditions.

**Equations:**
Using log-distance path loss model:

PL(d) = PL(d₀) + 10n log₁₀(d/d₀)

PL(10 m) = 40 dB + 10 × 2.7 × log₁₀(10) = 67 dB

ESP32 TX Power: +4 dBm  
WiFi Sensitivity: –90 dBm  
→ Received power = –63 dBm → Link margin = 27 dB

**Result:** WiFi communication is reliable across a 10-meter classroom with expected margin.

---

## 2025-04-29 – Final Integration and Debugging

**Objectives:**
- Complete testing loop from physical input to UI.
- Fix Firebase timestamp issues.

**Progress:**
- Verified timestamps now use `time(NULL) * 1000` for accurate ms precision.
- Fixed JSON encoding and logging in Firebase.
- Coordinated with hardware team to validate ESP32 behavior on PCB power-up.

<img width="1144" alt="Screenshot 2025-05-01 at 4 06 28 PM" src="https://github.com/user-attachments/assets/f437f391-97b4-4657-bfff-af872827cc5d" />

<img width="683" alt="Screenshot 2025-05-01 at 4 23 56 PM" src="https://github.com/user-attachments/assets/bcf6157d-593b-484a-bd59-469a2cab8ff9" />



**Remaining Tasks:**
- Final UI polish (labels, error states).
- Secure Firebase rules and access tokens.
- Conduct classroom-scale demo.

---

## Bibliographic References

1. [ESP32-S3-WROOM Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_datasheet_en.pdf)  
2. [Firebase Realtime Database Docs](https://firebase.google.com/docs/database)  
3. [Chart.js Documentation](https://www.chartjs.org/docs/)  
4. [Adafruit Push Button #492](https://www.adafruit.com/product/492)  
5. [Bourns PDB181 Datasheet](https://www.bourns.com/docs/Product-Datasheets/pdb181.pdf)

---

## Notes

- All application code (ESP32 firmware and web dashboard) was written by me.
- GitHub repo includes full commit history and test data.

