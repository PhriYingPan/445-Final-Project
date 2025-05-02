# Ethan Ge Worklog

[[_TOC_]]

# 2025-02-01 - Project Overview and Initial Design

Our team is developing a desk learning aid device to assist elementary school education. The system consists of:
- Physical device with buttons, rotary potentiometer, RFID module, and screen
- Web application for teacher interface

Key functionalities:
- Student response tracking via physical buttons
- Comprehension level indication (1-10 scale via potentiometer)
- RFID-based attendance system
- Real-time feedback display

As the sole EE major in the group, I'm taking lead on:
- Hardware design and PCB soldering
- Microcontroller programming
- Power system design

# 2025-02-14 - Hardware Design Decisions

## Communication Protocols
Initially planned:
- UART between input subsystem and microcontroller
- SPI for RFID communication

Final implementation:
- Changed to UART for RFID communication
- Using simple GPIO connections for inputs

## Physical Layout
Designed the device to be:
- Mountable to standard elementary school desks
- Unobtrusive (minimal desk space usage)
- Non-distracting for students

## Key Component Selections
- OLIMEX LTD BUTTON-12MM-RED
- TPS63001DRCR buck-boost converter
- Various supporting passive components

# 2025-03-01 - Power System Analysis

## Current Requirements
Component | Voltage | Max Current
--- | --- | ---
ESP32 S3 | 3.3V | 50mA (BLE mode)
RFID Module | 3.3V | 26mA
OLED Display | 3.3V | 20mA
**Total** | **3.3V** | **<100mA**

## Battery Calculations
