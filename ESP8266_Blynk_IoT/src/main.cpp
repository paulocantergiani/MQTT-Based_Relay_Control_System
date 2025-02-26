#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Blynk authentication and Wi-Fi credentials
char auth[] = "";
char ssid[] = "";
char pass[] = "";

// UART settings
#define UART_BAUD 115200
#define TX_PIN 1 // GPIO1
#define RX_PIN 3 // GPIO3

// Relay Commands
const uint8_t A1_ON[]  = {0xA0, 0x01, 0x01, 0xA2};
const uint8_t A1_OFF[] = {0xA0, 0x01, 0x00, 0xA1};
const uint8_t A2_ON[]  = {0xA0, 0x02, 0x01, 0xA3};
const uint8_t A2_OFF[] = {0xA0, 0x02, 0x00, 0xA2};
const uint8_t A3_ON[]  = {0xA0, 0x03, 0x01, 0xA4};
const uint8_t A3_OFF[] = {0xA0, 0x03, 0x00, 0xA3};
const uint8_t A4_ON[]  = {0xA0, 0x04, 0x01, 0xA5};
const uint8_t A4_OFF[] = {0xA0, 0x04, 0x00, 0xA4};

void sendCommand(const uint8_t *cmd, size_t len) {
  Serial.write(cmd, len);
}

// Virtual Pin V1 - Relay 1
BLYNK_WRITE(V1) {
  if (param.asInt() == 1) {
    sendCommand(A1_ON, sizeof(A1_ON));
  } else {
    sendCommand(A1_OFF, sizeof(A1_OFF));
  }
}

// Virtual Pin V2 - Relay 2
BLYNK_WRITE(V2) {
  if (param.asInt() == 1) {
    sendCommand(A2_ON, sizeof(A2_ON));
  } else {
    sendCommand(A2_OFF, sizeof(A2_OFF));
  }
}

// Virtual Pin V3 - Relay 3
BLYNK_WRITE(V3) {
  if (param.asInt() == 1) {
    sendCommand(A3_ON, sizeof(A3_ON));
  } else {
    sendCommand(A3_OFF, sizeof(A3_OFF));
  }
}

// Virtual Pin V4 - Relay 4
BLYNK_WRITE(V4) {
  if (param.asInt() == 1) {
    sendCommand(A4_ON, sizeof(A4_ON));
  } else {
    sendCommand(A4_OFF, sizeof(A4_OFF));
  }
}

void setup() {
  Serial.begin(UART_BAUD);
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();
}