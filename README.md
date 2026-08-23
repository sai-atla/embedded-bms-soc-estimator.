# embedded-bms-soc-estimator.
# Real-Time Embedded BMS & Li-ion State-of-Charge (SoC) Estimator

[![Wokwi Simulation](https://img.shields.io/badge/Wokwi-Live%20Simulation-brightgreen)](https://wokwi.com/projects/473171686381565953)
An embedded battery management sub-system built for real-time cell voltage telemetry, digital noise filtering, and non-linear State-of-Charge (SoC) estimation for Lithium-ion chemistries.

---

## Technical Specifications

| Parameter | Specification |
|---|---|
| **Microcontroller** | ATmega328P / Arduino Core |
| **ADC Resolution** | 10-bit Successive Approximation Register (SAR) |
| **Sampling Rate** | 5 Hz (200 ms non-blocking scheduling) |
| **Analog Front-End** | Precision 10kΩ / 10kΩ Divider + RC Anti-Aliasing Filter |
| **Digital Filtering** | 16-Sample Circular Moving Average Filter |
| **Telemetry Bus** | I2C (OLED Display @ 400 kHz) + UART (115200 Baud) |
| **Operating Range** | 3.00 V (0% Cut-off) to 4.20 V (100% Full Charge) |
## Circuit Architecture

[Li-ion Cell: 3.0V - 4.2V]
│
├──[ R1 = 10kΩ ]──┬────────────[ Arduino ADC (A0) ]
│                  │
├──[ R2 = 10kΩ ]   ├──[ 16-Tap Moving Avg Filter ]
│                  │
├──[ C1 = 100nF ]  └──[ Piecewise SoC Lookup ]
│                  │
GND                 ├──[ I2C SSD1306 OLED (A4/A5) ]
└──[ UART Serial @ 115200 ]


---

## Core Firmware Features

1. **Hardware-Level Noise Suppression:** High-frequency switching ripple is suppressed via a passive RC low-pass filter (fc ≈ 159 Hz).
2. **Circular Buffer DSP:** Implements an O(1) time complexity moving average filter over 16 samples to eliminate quantization jitter.
3. **Piecewise Empirical SoC Mapping:** 4-region curve fitting model based on actual Li-ion Open Circuit Voltage (OCV) discharge characteristics.
4. **Safety Watchdog:** Real-time comparator triggers a digital alert state when cell voltage drops below 3.30 V.

---
![BMS Circuit & Waveform](waveform bms.png)
## Author
**Sai Atla**  
Electrical & Electronics Engineering  
* [LinkedIn](https://linkedin.com/in/saiatla)
* [GitHub](https://github.com/sai-atla)
