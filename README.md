Here is the complete, single-file `README.md` incorporating the configuration steps and the post-flashing network management/captive portal instructions.

---

```markdown
# 🚀 ESP32-S3 Wi-Fi Extende

A high-performance **Wi-Fi Range Extender** firmware for **ESP32-S3** microcontrollers. This project utilizes native **NAPT (Network Address Port Translation)** to transparently extend Wi-Fi networks while hosting a professional, real-time **Glassmorphism Web Dashboard** directly on the device.

---

## ⚙️ Setup & Installation Before Flashing

### 1. Configuration
Before uploading the code, you must configure your network credentials. Open the source file (e.g., `main.cpp` or `ESP32S3_wifi_extender.cpp`) in the Arduino IDE and locate the **Configuration Section** at the top.

**Update the following lines with your specific details:**

```cpp
// --- Network Settings ---
#define STA_SSID        "Your_Home_Router"     // Input Network (The router you want to extend)
#define STA_PASS        "Router_Password"      // Password for the input router

#define AP_SSID         "ESP32-Extender-Pro"   // Output Network Name (Your new extended WiFi)
#define AP_PASS         "securepassword123"    // Password for your new extended WiFi

```

### 2. Flashing via Arduino IDE

*Note: Ensure you have the ESP32 board support package installed (v2.0.14 or newer).*

1. **Select Board:** Go to *Tools > Board* and select **ESP32S3 Dev Module**.
2. **Enable USB CDC:** Go to *Tools > USB CDC On Boot* and set it to **Enabled** (Required for Serial monitoring).
3. **Partition Scheme:** Go to *Tools > Partition Scheme* and select **Default 4MB with SPIFFS**.
4. **Upload:** Connect your ESP32-S3 via USB and click the **Upload** (Arrow) button in the IDE.

---

## 📊 Connecting and Managing after Flashing

Once the code is successfully uploaded, follow these steps to connect to your new extender and access the management dashboard.

### 1. Power On

Disconnect the ESP32 from your computer and connect it to a stable USB power source (e.g., a 5V/2A phone charger).

### 2. Connect to the New Network

On your phone, tablet, or laptop, open your Wi-Fi settings. Look for the new network name you defined in the configuration step (e.g., **`Your ESP32-Extender`**) and connect using the password you set.

### 3. Access the Captive Portal Dashboard

The device uses a **Captive Portal** to automatically direct you to the dashboard.

* **Automatic:** On most mobile devices, a notification like "Sign in to network" will appear. Tapping this will open the Glassmorphism Dashboard automatically.
* **Manual:** If the dashboard does not open automatically, open any web browser and navigate to **`http://192.168.4.1`**.

### 4. Verify Connectivity

Once on the dashboard, check the "System Vitality" card. The Internet status should show a green **Online** indicator, confirming the extender has successfully connected to your main router and is ready for use.

---

### 👨‍💻 Author

**PaulsonAshish Errala**

```

```
