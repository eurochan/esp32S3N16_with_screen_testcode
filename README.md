# 🚀 ESP32 Smart Pocket Card

![Status](https://img.shields.io/badge/Status-Work_in_Progress-yellow?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-ESP32_SuperMini-blue?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)

> **A compact, intelligent, and portable IoT companion built on the ESP32 SuperMini.**

## 📖 Overview

This project aims to create a sleek, portable smart card that bridges the gap between a daily accessory and a powerful IoT device. Powered by the ultra-compact **ESP32 SuperMini**, this card is designed to be energy-efficient, context-aware, and helpful in your daily routine.

Whether it's checking the time with a simple gesture or automating peace-of-mind notifications when you arrive at your destination, the **Smart Pocket Card** handles it silently in the background.

---

## ✨ Key Features

### ⌚ Smart Wake-on-Lift
No buttons needed. Utilizing motion detection (IMU/Tilt Sensor), the device intelligently detects when you pick it up and instantly lights up the display to show the current time.
*   *Energy efficient deep-sleep mode when idle.*
*   *Instant response.*

### 📍 Proximity Automation (WiFi Geofencing)
Forget sending "I'm here" texts manually. The device passively scans for specific WiFi networks (e.g., School or Office).
*   **Trigger:** Detects connection to the target WiFi SSID.
*   **Action:** Automatically sends a "Safe Arrival" notification to parents/guardians via Webhook (Telegram, Line, Discord, or SMS).

---

## 🛠️ Hardware Specs

| Component | Model / Description | Status |
| :--- | :--- | :--- |
| **MCU** | ESP32 SuperMini (C3/S3) | ✅ Ready |
| **Display** | 0.96" OLED / E-Paper (TBD) | 🚧 TBD |
| **Sensor** | MPU6050 / SW-520D (Tilt) | 🚧 TBD |
| **Power** | LiPo Battery + TP4056 Charger | 🚧 TBD |

---

## 🗺️ Roadmap & Future Ideas

This project is currently in the **Initialization Phase**. Here is what's coming next:

- [ ] **Core:** Basic hardware assembly and deep-sleep optimization.
- [ ] **Feature:** Implement NTP Time Sync logic.
- [ ] **Feature:** Develop the "WiFi Sniffer" logic for arrival detection.
- [ ] **Expansion:** Add a "Panic Button" for emergency alerts.
- [ ] **Expansion:** Display weather info upon waking up.
- [ ] **Expansion:** NFC/RFID integration for access control.

---

## 🚀 Getting Started

*(Instructions on how to flash the firmware will be added here)*

```bash
# Clone the repository
git clone https://github.com/yourusername/esp32-smart-card.git
