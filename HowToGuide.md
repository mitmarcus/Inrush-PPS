# Inrush-PPS: User & Operation Guide

This guide covers the setup, operation, and safety procedures for the **Inrush-PPS** system.

---

## 1. Physical Assembly & Wiring

Before powering on, ensure the hardware is connected according to the following map:

### Stacking

1.  Plug the **M5Stack CoreS3 SE** directly onto the **PPS Module** on the internal bus.
2.  Ensure the pins are aligned correctly to avoid bending the header.

### Connections

- **DC Input:** Connect a 12V–36V DC power source on the PPS Module.
- **Sensor (INA226):** Use a Grove cable to connect the **INA226 Unit** to **Port A** (Red) on the CoreS3 SE.
- **Switch (MOSFET):** Connect the **DFRobot MOSFET** signal pin to **GPIO 5** (needs poewr and gorund as well).

---

## 2. Powering On & Network Setup

1.  Apply DC power to the PPS module. The CoreS3 SE will boot automatically.
2.  **WiFi Connection:** The device will attempt to connect to the SSID configured in the firmware.
3.  **Find the IP:** Once connected, the local IP address will be displayed on the CoreS3 SE screen (press the button to show).
4.  **Access Dashboard:** Open a web browser on your PC or smartphone and type in the IP address (e.g., `http://192.168.1.50`).

---

## 3. Operating the Dashboard

### Manual Mode

- **Set Voltage:** Use the slider or number input to choose your target voltage (10V–30V). _I set it to 10V but it can do from 0.5V_
- **PPS ON/OFF:** This enables the internal buck converter. The voltage will reach the output terminals, but the load is still disconnected.
- **MOSFET ON/OFF:** Use this to "slam" the power into your DUT. This is how you test **Inrush Current**.
  - _Tip: Turn PPS ON first, wait for the voltage to stabilize, then toggle the MOSFET ON to capture the peak spike._

### CSV Replay Mode

1.  **Upload:** Drag and drop a CSV file into the upload zone. Format: `YYYY-MM-DDTHH:MM:SS.mmmZ, Voltage`.
2.  **Playback Speed:** Adjust the speed (from 0.25x for high detail to 128x for fast stress testing).
3.  **Range Markers:** Click on the graph to set an **IN** point (Green) and Right-Click to set an **OUT** point (Red). The system will only play/loop within this window. Middle click to reset _We all have thinkpads here so it's easy_
4.  **Loop:** Toggle the Loop button to repeat the profile indefinitely.

---

## 4. Thermal Management (Critical)

The PPS Module and MOSFET are compact components capable of handling up to 100W, which generates significant heat.

- **Short Bursts (< 10 minutes):** No external cooling is typically required.
- **Long-Duration Testing / High Current:** **You must use an external cooling fan.**
  - Point a 40mm or 120mm fan directly at the PPS Module's heatsink and the DFRobot MOSFET.
  - Monitor the "Temp" reading on the dashboard. If the temperature exceeds **70°C**, the module may throttle or shut down for safety.
- **Continuous 100W Load:** If running at the 100W limit for more than a few minutes, active airflow is mandatory to prevent permanent hardware damage.

---

## 5. Troubleshooting

| Issue                     | Potential Cause                      | Solution                                                                                                                                                                                 |
| :------------------------ | :----------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **"PPS Offline" Error**   | Internal I2C bus hang or power loss. | Check power connection; reboot the CoreS3.                                                                                                                                               |
| **"INA226 Offline"**      | Grove cable loose.                   | Re-seat the cable in Port A.                                                                                                                                                             |
| **Voltage Drop at Load**  | Resistance in thin wires.            | The software includes **Diode-Drop Compensation** that is used only as a backup. Ensure the INA226 is connected so the system can "see" the drop and boost the PPS output automatically. |
| **Dashboard not loading** | IP address mismatch.                 | Verify the IP displayed on the CoreS3 screen matches your browser URL.                                                                                                                   |

---

## 6. CSV Format Example

Your CSV should follow this structure for the parser to read it correctly:

```csv
Timestamp,Voltage
2024-01-01T00:00:00.000Z,12.0
2024-01-01T00:00:00.500Z,12.5
2024-01-01T00:00:01.000Z,13.0
```
