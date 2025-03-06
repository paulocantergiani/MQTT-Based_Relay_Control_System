# IoT MQTT Relay Control System

This project provides a complete, secure solution for remotely controlling relays using MQTT over AWS IoT. It integrates microcontroller firmware for ESP32/ESP8266 devices with a robust Python web application, enabling real-time, cloud-connected relay operations.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Setup and Installation](#setup-and-installation)
  - [AWS IoT Configuration](#aws-iot-configuration)
  - [Microcontroller Firmware Setup](#microcontroller-firmware-setup)
  - [Web Application Setup](#web-application-setup)
  - [Deployment on Render](#deployment-on-render)
  - [Database Setup](#database-setup)
- [MQTT Communication](#mqtt-communication)
- [Security](#security)
- [License](#license)

## Overview

This system allows users to control relays remotely through a web interface while ensuring secure communication via AWS IoT. The system comprises:
- **Microcontroller Firmware:** Two versions tailored for ESP32 and ESP8266, which subscribe to MQTT topics to control relays.
- **Web Application:** A Python-based platform that handles user authentication, MQTT messaging, and logs interactions in a PostgreSQL database.

## Features

- **Remote Control:** Manage relays through a simple, intuitive web interface.
- **Secure Communication:** Uses AWS IoT and TLS encryption for robust MQTT messaging.
- **User Authentication:** Integrated with Flask-Login for secure access control.
- **Real-Time Monitoring and Logging:** Logs user actions and relay status to a PostgreSQL database.
- **Scalability:** Easily supports multiple relay devices with dedicated MQTT topics.
- **Cloud Deployment:** Ready for deployment on platforms like Render with minimal configuration.

## Architecture

The system is designed with a clear separation between hardware control and web-based management:
- **Hardware Layer:** ESP32/ESP8266 devices running firmware to listen for MQTT commands and toggle relay states.
- **Cloud Layer:** AWS IoT acts as the MQTT broker ensuring secure, scalable, and reliable message delivery.
- **Application Layer:** A Python web application provides the user interface, authentication, and logging. It communicates with AWS IoT to control the hardware in real-time.

## Project Structure

```plaintext
.
├── ESP32_MQTT_Receiver/             # Firmware for ESP32-based devices
│   └── main.cpp                     # Example code: WiFi, MQTT setup, and relay control
├── ESP8266_MQTT_Receiver/           # Firmware for ESP8266-based devices
│   └── main.cpp                     # Similar functionality as ESP32 version
├── WebApp_Python_MQTT_Client/       # Python web application for relay control
│   ├── app.py                       # Main Flask application
│   ├── requirements.txt             # Python dependencies
│   ├── templates/                   # HTML templates for the web UI
│   ├── static/                      # Static assets (CSS, JS)
│   └── certs/                       # AWS IoT certificate files
└── README.md                        # Project documentation
```

## Setup and Installation

### AWS IoT Configuration

1. **Register Your Device:**
   - Sign up for AWS IoT.
   - Register your device (Thing) and download the AWS Root CA, device certificate, and private key.

2. **Policy Setup:**
   - Create a policy with the following permissions:
     ```json
     {
       "Version": "2012-10-17",
       "Statement": [
         { "Effect": "Allow", "Action": "iot:Connect", "Resource": "*" },
         { "Effect": "Allow", "Action": "iot:Publish", "Resource": "*" },
         { "Effect": "Allow", "Action": "iot:Receive", "Resource": "*" },
         { "Effect": "Allow", "Action": "iot:Subscribe", "Resource": "*" }
       ]
     }
     ```
   - Attach the policy to your device certificate.

### Microcontroller Firmware Setup

1. **Tools Required:**  
   Use the Arduino IDE or PlatformIO.

2. **Configuration:**
   - Open `main.cpp` in either the ESP32 or ESP8266 directory.
   - Update the code with your:
     - WiFi credentials
     - AWS IoT endpoint
     - Paths to certificate files
     - Desired MQTT topics (e.g., `gates/gate1/control`)
     
3. **Upload Firmware:**  
   Flash the updated firmware onto your ESP32/ESP8266 device.

### Web Application Setup

1. **Environment:**
   - Ensure Python 3.x is installed.
   - Install dependencies:
     ```bash
     pip install -r requirements.txt
     ```
     
2. **AWS Certificates:**
   - Place your AWS IoT certificate files in the `certs/` directory.

3. **Run the Application Locally:**
   - Start the Flask server:
     ```bash
     python3 app.py
     ```
   - Open your browser and navigate to `http://localhost:5000`.

### Deployment on Render

1. **Prepare Your Repository:**
   - Ensure all necessary files (Flask app, MQTT client code, HTML, static assets) are included.

2. **Create a Procfile:**
   - Add a file named `Procfile` with the following:
     ```plaintext
     web: gunicorn --bind 0.0.0.0:$PORT app:app
     ```

3. **Render Dashboard Setup:**
   - Log into Render.
   - Create a new Web Service connected to your repository.
   - Configure the branch, build command (`pip install -r requirements.txt`), start command (from the Procfile), and set environment variables (`SECRET_KEY`, `DATABASE_URL`, `MQTT_BROKER`, etc.).

### Database Setup

1. **PostgreSQL Instance:**
   - Create a PostgreSQL instance (e.g., using Render's dashboard).
   
2. **Environment Variable:**
   - Set the connection string as `DATABASE_URL`.

3. **Database Initialization:**
   - Run the provided database initialization script (e.g., `create_db.py`) to create the required tables.

## MQTT Communication

The system leverages MQTT topics to control relays:
- **Example Topics:**
  - `gates/gate1/control`: Accepts commands like `"turn_on"` or `"turn_off"`.
  - `gates/gate2/control`: Used for additional relay devices.
  
This topic structure ensures organized and scalable communication across multiple devices.

## Security

- **User Authentication:**  
  Managed through Flask-Login ensuring only authenticated users can control relays.

- **Data Protection:**  
  TLS encryption is used for all MQTT communications to secure data in transit.

- **Rate Limiting:**  
  Implemented via Redis to prevent abuse and ensure system reliability.

## License

This project is distributed under the [MIT License](LICENSE).