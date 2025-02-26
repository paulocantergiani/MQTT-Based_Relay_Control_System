# MQTT-Based Relay Control System

This project implements a **relay control system** using **ESP8266** and/or **ESP32** as subscribers, which can be controlled via an **internet-hosted web application**. The system uses **AWS IoT** as the MQTT broker and allows remote operation of relays.

## Features
- **Control relays remotely** via a web interface.
- Uses **MQTT protocol** for communication.
- **Python/JavaScript WebApp** for internet accessibility.
- **User authentication** and security features.
- **Supports multiple relays** via AWS IoT MQTT.

## System Architecture

```
+-------------------+       MQTT       +-------------------+
|  Web Application | <----------------> |  AWS IoT Broker  |
+-------------------+                   +-------------------+
       |                                          |
       | HTTP                                     | MQTT
       v                                          v
+-------------------+                   +-------------------+
|  Flask Backend   |                   | ESP8266/ESP32 MCU |
|  (Python)        |                   |   (Gate Control)  |
+-------------------+                   +-------------------+
```

## Project Structure
```
.
├── ESP32_MQTT_Receiver/             # Code for ESP32-based MQTT client wired to the relays
├── ESP8266_MQTT_Receiver/           # Code for ESP8266-based MQTT client wired to the relays
├── ESP8266_Blynk_IoT/               # Integration with Blynk as an alternative for AWS Iot MQTT communication
├── WebApp_JavaScript_MQTT_Client/   # Web app (JavaScript-based)
├── WebApp_Python_MQTT_Client/       # Web app (Python-based)
└── README.md                        # This file
```

## Setup Instructions

### 1. AWS IoT Setup
#### Steps:
   - Create a AWS IoT root user account
   - Create a "Thing"
   - Create a Certificate
   - Create a Policy with the following configuration:
```sh
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "*"
    }
  ]
}
```
   - Attach the Policy to the Certificate and then the Certificate to the Thing
   - **Note:** Save the certificates that will be prompt to you and add them to your code later.

### 2. Web Application (Flask Backend)
#### Prerequisites:
- Python 3.x
- Flask
- Redis (for rate limiting)
- AWS IoT MQTT SDK

#### Install dependencies:
```sh
pip install flask flask-login flask-wtf flask-limiter redis bcrypt AWSIoTPythonSDK
```

### Upload the AWS Certificates to the '/certs' folder

#### Run the application locally on your computer:
```sh
python3 app.py
```
The web app will be available at `http://localhost:5000`.

---

### 3. ESP8266/ESP32 Setup
#### Prerequisites:
- **Arduino IDE** or **PlatformIO**
- ESP8266 or ESP32 board

#### Steps:
1. Open `main.cpp`.
2. Modify:
   - **WiFi Credentials**
   - **AWS IoT Endpoint**
   - **AWS Certificates**
   - Change the **MQTT Topics** for your use
3. Upload the code to the board.
4. The microcontroller will automatically connect to WiFi and MQTT.

---

## MQTT Topics used on this project
| Topic Name             | Description                     | Payload |
|------------------------|---------------------------------|---------|
| `gates/gate1/control`  | Control relay for Gate 1       | `"turn_on"` / `"turn_off"` |
| `gates/gate2/control`  | Control relay for Gate 2       | `"turn_on"` / `"turn_off"` |

---

## Security Features
- **User authentication** with Flask-Login.
- **Rate limiting** using Redis.
- **TLS encryption** for secure MQTT communication.

---

## License
This project is open-source and available under the **MIT License**.