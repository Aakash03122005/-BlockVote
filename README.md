# BlockVote – ESP32-S3 Based Secure Electronic Voting Kiosk

BlockVote is an **IoT-enabled electronic voting system** built on the ESP32-S3 platform, integrating **biometric fingerprint authentication**, a **touchscreen voting interface**, and **secure vote handling logic**.  
The system is designed to ensure **voter authenticity, transparency, and reliability**, even in **low-connectivity environments**.

---

## 🔐 Key Features

- Biometric voter authentication using **R307 fingerprint sensor**
- Touch-based voting UI with **2×3 candidate grid**
- Secure vote increment only after **valid fingerprint match**
- Visual feedback using **modal dialogs and LED indicators**
- Indian-flag themed UI background for national election context
- Designed for **offline-first operation**, suitable for remote locations
- Modular codebase for future **blockchain integration**

---

## 🧠 System Architecture

1. User selects a party on the touchscreen
2. System prompts for fingerprint authentication
3. Fingerprint is matched against stored templates
4. Vote is recorded only if authentication succeeds
5. Visual confirmation is shown on display
6. Vote count is updated securely in memory

---

## 🧩 Hardware Components

| Component | Description |
|--------|------------|
| ESP32-S3 Dev Kit | Main microcontroller |
| ILI9341 TFT Display | SPI-based 240×320 touchscreen |
| XPT2046 | Touch controller |
| R307 Fingerprint Sensor | Optical biometric authentication |
| LEDs | Green (success) & Red (error) indicators |
| Power Supply | 5V regulated |

---

## 🔌 Pin Configuration

### TFT Display (ILI9341)
- CS  → GPIO 10  
- DC  → GPIO 8  
- RST → GPIO 9  

### Touch Controller (XPT2046)
- CS  → GPIO 4  
- IRQ → GPIO 5  

### Fingerprint Sensor (R307)
- TX → GPIO 16 (ESP32 RX)
- RX → GPIO 17 (ESP32 TX)
- Baud Rate → 57600

### LEDs
- Green LED → GPIO 18  
- Red LED   → GPIO 21  

---

## 🛠 Software Stack

- **Platform:** ESP32-S3
- **Language:** C++ (Arduino Framework)
- **Libraries Used:**
  - Adafruit_GFX
  - Adafruit_ILI9341
  - XPT2046_Touchscreen
  - Adafruit_Fingerprint
  - SPI

---

## 📋 Functional Highlights

- **White modal dialogs** for authentication, success, and error states
- **Touch calibration support** for accurate input
- **Vote count displayed live** on UI
- **Debounce handling** for touch events
- LED-based feedback for accessibility

---

## 🚀 How to Run

1. Connect all hardware components as per pin configuration
2. Install required Arduino libraries
3. Select **ESP32-S3 board** in Arduino IDE
4. Upload the code to the ESP32-S3
5. Enroll fingerprints using R307 enrollment sketch
6. Start voting via touchscreen

---

## 🔒 Security Considerations

- Votes are recorded **only after biometric verification**
- No vote is counted without a valid fingerprint match
- Designed to be extended with **blockchain-based immutable storage**

---

## 📌 Future Enhancements

- Blockchain integration for immutable vote storage
- Secure backend synchronization
- Voter ID hashing & encryption
- Audit trail generation
- Admin dashboard for election monitoring

---

## 📄 Research & Publication

This project has been **submitted for peer review** to:

**International Conference for Convergence in Computing Technology (ICCCT 2026)**  
Paper ID: 2931  
Status: *Under Review*

---

## 👨‍💻 Author

**Aakash Dabhade**  
Electronics & Telecommunication Engineering  
Vishwakarma Institute of Technology, Pune  

🔗 GitHub: https://github.com/Aakash03122005  

---

## ⚠️ Disclaimer

This project is intended for **educational and research purposes only**.  
Not certified for use in real governmental elections.
