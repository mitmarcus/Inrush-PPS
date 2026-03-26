# Inrush-PPS

**Inrush-PPS** is a programmable power supply (PPS) and characterization system designed for testing the inrush current and transient response of electronic devices. 

By combining high-precision voltage control with a high-speed MOSFET switching stage and high-resolution current monitoring, this tool allows for repeatable, automated power-on testing and voltage-curve "replays."

<img alt="photo_1_2026-03-26_09-31-08" src="https://github.com/user-attachments/assets/ed9be583-4783-41a0-878b-7feca3c43d99" />

## 🚀 Key Features!

### 💻 Industrial Web Dashboard (NodeATE)
*   **Live Monitoring:** High-contrast 7-segment display for output voltage.
*   **Real-time Graphing:** Visualizes voltage curves with interactive playheads and tooltips.
*   **Remote Control:** Full control of the PPS state, voltage levels, and MOSFET switching via any web browser.
*   **Mobile Responsive:** Optimized for both desktop and mobile field use.

### 📈 CSV Replay & Profiling
*   **Voltage Playback:** Upload CSV files to "replay" specific voltage profiles (simulating battery drain, brownouts, or specific power-up sequences).
*   **Variable Speed:** Playback curves from **0.25x to 128x** real-time speed.
*   **Range Markers:** Select specific segments of a CSV to loop or test.
*   **Persistent Storage:** CSVs and test states are saved to the onboard **LittleFS** (Flash memory), surviving reboots and power cycles.

### 🛡️ Robustness & Precision
*   **Diode-Drop Compensation:** Automatically calculates and compensates for voltage drops across the MOSFET and wiring path using the INA226 sensor.
*   **Power-Loss Recovery:** Automatically resumes a test sequence from the last known state if power is interrupted.
*   **Auto I2C Recovery:** Self-healing communication bus logic to prevent system hangs in high-EMI environments.
*   **Watchdog Protection:** Integrated hardware watchdog (WDT) ensures the system remains responsive during long-term testing.

<img width="1920" height="1276" alt="screencapture-192-168-1-228-2026-03-26-09_53_59" src="https://github.com/user-attachments/assets/4937859b-e03c-4dbc-b04e-a207d1a2aa12" />

---

## 🛠 Hardware Components

| Component | Description | Link |
| :--- | :--- | :--- |
| **M5Stack CoreS3 SE** | Main ESP32-S3 controller with touch display. | [Shop M5Stack](https://shop.m5stack.com/products/m5stack-cores3-se-iot-controller-without-battery-bottom) |
| **M5Stack PPS Module** | Programmable DC-DC (0.5V–30V @ 5A, 100W). | [Shop M5Stack](https://shop.m5stack.com/products/programmable-power-supply-module-13-2) |
| **M5Stack INA226 (10A)** | High-precision I2C Voltage/Current sensor. | [Shop M5Stack](https://shop.m5stack.com/products/ina226-10a-current-voltage-power-monitor-unit) |
| **DFRobot MOSFET** | Gravity-series MOSFET for high-speed switching. | [Shop DFRobot](https://www.dfrobot.com/product-1567.html) |
| A diode | Any diode that would work for this power | |
| Kemet 8200uf| 63VDC AL871A Big capacitor I had on hands | |
| Banana plugs | Any plugs you like | |
| Safety switch | For easy restarting of the device | |
| Wire/Connectors | I would recommend very soft and no lower than 24 gauge; as for connectors I love Wago||
---

## 🏗 System Architecture

1.  **Logic:** The **CoreS3 SE** serves as the brain, running the web server and controlling the PPS module via the internal I2C bus.
2.  **Power:** The **PPS Module** regulates input power (12V-36V) to the desired test voltage.
3.  **Switching:** The **DFRobot MOSFET** (connected to GPIO 5) acts as the high-speed gate to trigger "inrush" events.
4.  **Sensing:** The **INA226** provides high-speed current monitoring (up to 10A) to capture the magnitude of the inrush spike.

---

## 📖 Getting Started

### 1. Setup Environment
*   Install the **M5Unified**, **M5UnitUnified**, and **M5UnitUnifiedMETER** libraries in the Arduino IDE or PlatformIO.
*   Ensure your Partition Scheme is set to **"Default 4MB with spiffs"** or **"LittleFS"** to enable CSV storage.

### 2. Wiring
*   Stack the CoreS3 SE on the PPS Module.
*   Connect the INA226 to the Grove Port A.
*   Connect the MOSFET Signal to GPIO 5.
*   Connect your Load through the MOSFET and INA226.

### 3. Usage
*   Power on the device and connect to the local WiFi (configured in the code).
*   Access the dashboard via the IP address shown on the CoreS3 screen.
*   Upload a CSV (Format: `Timestamp, Voltage`) to begin automated characterization.

## ⚠️ Safety Warning
This system can handle up to **150W** (in bursts) of power. Normally the max is 100W. Ensure all cables and connectors are rated for high current. The MOSFET may require additional heatsinking if used for continuous high-load testing. Always test with a current limit set in the software.

