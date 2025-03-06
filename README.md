# IoT MQTT Relay Control System

This project provides a comprehensive solution for remotely controlling relays using MQTT over AWS IoT. It combines a Python-based web application with ESP8266/ESP32 microcontrollers to enable secure, cloud-connected relay operations.

## Key Features

- **Remote Relay Operation:** Easily control relays via an intuitive web interface.
- **Secure Communication:** Utilizes AWS IoT with MQTT for robust, encrypted messaging.
- **User Authentication:** Features secure user login and profile management using Flask-Login.
- **Cloud-Integrated Database:** Stores user data and activity logs in an online PostgreSQL database.
- **Scalable Architecture:** Supports multiple relay devices with detailed logging and real-time control.

## Project Structure

```
.
├── ESP32_MQTT_Receiver/             # ESP32-based MQTT client code for relay control
├── ESP8266_MQTT_Receiver/             # ESP8266-based MQTT client code for relay control
├── WebApp_Python_MQTT_Client/         # Python web application integrating MQTT
└── README.md                          # Project documentation
```

## Setup and Installation

### 1. AWS IoT Configuration

Set up your AWS IoT environment by following these steps:
- **Create an AWS IoT Account & Thing:** Sign up for AWS IoT and register your device (Thing).
- **Generate and Download Certificates:** Create a certificate for your device and download the AWS Root CA, device certificate, and private key.
- **Configure a Policy:** Create a policy allowing the following actions:
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
- **Attach the Policy:** Link the policy to your certificate and attach the certificate to your Thing.
- **Keep Your Certificates Safe:** You’ll use these files in both your web application and microcontroller configurations.

### 2. Microcontroller Setup (ESP8266/ESP32)

To prepare your device:
- **Required Tools:** Arduino IDE or PlatformIO with your ESP8266/ESP32 board.
- **Update Configuration:** Open `main.cpp` and modify:
  - WiFi credentials
  - AWS IoT endpoint
  - Certificate details (paths to AWS Root CA, device certificate, and key)
  - MQTT topics as needed
- **Flash the Firmware:** Upload the modified code to your board. The microcontroller will automatically connect to Wi-Fi and AWS IoT via MQTT.

### 3. Running the Web Application Locally

- **Prerequisites:** Ensure you have Python 3.x installed.
- **Install Dependencies:** Run:
  ```bash
  pip install -r requirements.txt
  ```
- **AWS Certificates:** Place your AWS IoT certificate files in the `/certs` folder as referenced in the code.
- **Start the Application:** Launch the web server by executing:
  ```bash
  python3 app.py
  ```
- **Access the App:** Open your browser and navigate to `http://localhost:5000`.

### 4. Deployment on Render

Deploy your application to the cloud using Render by following these steps:
- **Prepare Your Repository:** Make sure your repo includes:
  - The Flask web application
  - MQTT client code
  - HTML templates and static assets
- **Create a Procfile:** Add a file named `Procfile` with the following content:
  ```bash
  web: gunicorn --bind 0.0.0.0:$PORT app:app
  ```
- **Set Up on Render:**
  1. Log into the [Render Dashboard](https://dashboard.render.com/).
  2. Create a new Web Service and connect your Git repository.
  3. Configure:
     - **Branch:** For example, `main`
     - **Build Command:** Render will run `pip install -r requirements.txt`
     - **Start Command:** As specified in your Procfile
     - **Environment Variables:** Include `SECRET_KEY`, `DATABASE_URL`, `MQTT_BROKER`, and certificate paths.
- **Deploy:** Push your changes to trigger a deployment and monitor logs via the Render dashboard.

### 5. PostgreSQL Database Setup

- **Database Creation:** Create a PostgreSQL instance using Render’s dashboard.
- **Environment Variable:** Add the database connection string as `DATABASE_URL` either locally or in your Render project settings.
- **Create the DataBase Structure:** Run the create_db.py locally once to create the database.

## MQTT Topics Overview

| Topic                  | Description                  | Expected Payload          |
|------------------------|------------------------------|---------------------------|
| `gates/gate1/control`  | Relay control for Gate 1     | `"turn_on"` / `"turn_off"`|
| `gates/gate2/control`  | Relay control for Gate 2     | `"turn_on"` / `"turn_off"`|

## Security and Authentication

- **User Management:** Secure authentication and session management using Flask-Login.
- **Rate Limiting:** Implemented via Redis to mitigate abuse.
- **TLS Encryption:** All MQTT communications are secured with AWS IoT certificates.

## License

This project is open-source and distributed under the **MIT License**.