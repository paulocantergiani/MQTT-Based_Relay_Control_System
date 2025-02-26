#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <PubSubClient.h>
#include <time.h>

// ===================== USER CONFIGURATION =======================

// WiFi credentials
const char *WIFI_SSID = "S21 FE";
const char *WIFI_PASSWORD = "abcdefgh";

// AWS IoT Endpoint
const char *AWS_IOT_ENDPOINT = "";

// MQTT port for AWS IoT (usually 8883)
const uint16_t AWS_IOT_PORT = 8883;

// =============== Embedding Certificates & Keys =================
// Paste your certificates *verbatim* between the lines below.
// It's best to put them in PROGMEM to save RAM on ESP8266.

static const char AWS_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

static const char DEVICE_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

static const char DEVICE_KEY[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";

// =========== FORWARD DECLARATIONS =====================
void connectToWiFi();
void syncTime();
void setupTLSCredentials();
void connectToAWS();
void mqttCallback(char *topic, byte *payload, unsigned int length);

void sendCommand(const uint8_t *cmd, size_t len);

// ============= GLOBALS =================================
BearSSL::WiFiClientSecure net;
PubSubClient mqttClient(net);

unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds

// ============= RELAY COMMANDS (for 4 gates) ============
const uint8_t A1_ON[] = {0xA0, 0x01, 0x01, 0xA2};
const uint8_t A1_OFF[] = {0xA0, 0x01, 0x00, 0xA1};
const uint8_t A2_ON[] = {0xA0, 0x02, 0x01, 0xA3};
const uint8_t A2_OFF[] = {0xA0, 0x02, 0x00, 0xA2};
const uint8_t A3_ON[] = {0xA0, 0x03, 0x01, 0xA4};
const uint8_t A3_OFF[] = {0xA0, 0x03, 0x00, 0xA3};
const uint8_t A4_ON[] = {0xA0, 0x04, 0x01, 0xA5};
const uint8_t A4_OFF[] = {0xA0, 0x04, 0x00, 0xA4};

// =======================================================

void setup()
{
  Serial.begin(115200);

  // A small splash banner
  Serial.println();
  Serial.println("=========================================================");
  Serial.println("           ESP8266 Relay Controller - Booting           ");
  Serial.println("=========================================================");
  delay(500);

  // Connect to Wi-Fi
  connectToWiFi();

  // Sync system time via NTP (essential for TLS)
  syncTime();

  // Set up the TLS credentials from code
  setupTLSCredentials();

  // Initialize the MQTT client
  mqttClient.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  mqttClient.setCallback(mqttCallback);

  // Attempt initial AWS connection
  connectToAWS();
}

void loop()
{
  // If WiFi drops, attempt to reconnect
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("***************************************");
    Serial.println("[WiFi] WiFi Disconnected, reconnecting...");
    Serial.println("***************************************");
    connectToWiFi();
  }

  // If MQTT not connected, try reconnecting periodically
  if (!mqttClient.connected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL)
    {
      lastReconnectAttempt = now;
      connectToAWS();
    }
  }
  else
  {
    mqttClient.loop(); // handle incoming messages
  }

  delay(100);
}

// ===================================================================
// 1) Connect to WiFi
void connectToWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  Serial.println();
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(WIFI_SSID);
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  // Wait up to 20 seconds for connection
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    Serial.println("[WiFi]         Connected to Wi-Fi Successfully!        ");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  }
  else
  {
    Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    Serial.println("[WiFi]    FAILED to connect. (Check credentials?)      ");
    Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
  }
}

// 2) Sync time via NTP
void syncTime()
{
  Serial.println();
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  Serial.println("[Time] Synchronizing via NTP...");
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  Serial.println("=========================================================");
  Serial.print("[Time] NTP sync complete - Current time: ");
  Serial.println(ctime(&now));
  Serial.println("=========================================================");
}

// 3) Setup TLS credentials (from code constants)
void setupTLSCredentials()
{
  Serial.println();
  Serial.println("********************************************************");
  Serial.println("[TLS] Loading certificates from PROGMEM...");
  Serial.println("********************************************************");

  // Optionally reduce memory usage
  net.setBufferSizes(1024, 1024);

  // Convert certificate strings from PROGMEM to normal char arrays
  BearSSL::X509List *caCert = new BearSSL::X509List(AWS_ROOT_CA);
  BearSSL::X509List *clientCert = new BearSSL::X509List(DEVICE_CERT);
  BearSSL::PrivateKey *privKey = new BearSSL::PrivateKey(DEVICE_KEY);

  net.setTrustAnchors(caCert);
  net.setClientRSACert(clientCert, privKey);

  Serial.println("[TLS] Certificates loaded successfully.");
  Serial.println("********************************************************");
}

// 4) Connect to AWS IoT
void connectToAWS()
{
  if (mqttClient.connected())
    return;

  Serial.println();
  Serial.println("---------------------------------------------------------");
  Serial.println("[MQTT] Attempting to connect to AWS IoT...");
  Serial.println("---------------------------------------------------------");

  // Provide a unique client ID
  if (mqttClient.connect("esp8266_relays_controller"))
  {
    Serial.println("[MQTT] Successfully connected to AWS IoT!");

    // Subscribe to topics that control each gate
    mqttClient.subscribe("gates/gate1/control");
    mqttClient.subscribe("gates/gate2/control");
    mqttClient.subscribe("gates/gate3/control");
    mqttClient.subscribe("gates/gate4/control");

    Serial.println("[MQTT] Subscribed to gate control topics.");
    Serial.println("---------------------------------------------------------");
  }
  else
  {
    Serial.print("[MQTT] Connection failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" (Check root CA, Cert, and Key?)");
    Serial.println("---------------------------------------------------------");
  }
}

// 5) MQTT Callback
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  // Convert raw payload to a String
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.println("=========================================================");
  Serial.print("[MQTT] Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("[MQTT] Payload: ");
  Serial.println(message);
  Serial.println("=========================================================");

  // If we expect "1" or "0" for ON/OFF:
  bool turnOn = (message == "1");

  // Determine which gate topic was triggered
  if (strcmp(topic, "gates/gate1/control") == 0)
  {
    if (turnOn)
    {
      sendCommand(A1_ON, sizeof(A1_ON));
    }
    else
    {
      sendCommand(A1_OFF, sizeof(A1_OFF));
    }
  }
  else if (strcmp(topic, "gates/gate2/control") == 0)
  {
    if (turnOn)
    {
      sendCommand(A2_ON, sizeof(A2_ON));
    }
    else
    {
      sendCommand(A2_OFF, sizeof(A2_OFF));
    }
  }
  else if (strcmp(topic, "gates/gate3/control") == 0)
  {
    if (turnOn)
    {
      sendCommand(A3_ON, sizeof(A3_ON));
    }
    else
    {
      sendCommand(A3_OFF, sizeof(A3_OFF));
    }
  }
  else if (strcmp(topic, "gates/gate4/control") == 0)
  {
    if (turnOn)
    {
      sendCommand(A4_ON, sizeof(A4_ON));
    }
    else
    {
      sendCommand(A4_OFF, sizeof(A4_OFF));
    }
  }
}

// ============== Relay Command Sender ==================
void sendCommand(const uint8_t *cmd, size_t len)
{
  // For demonstration, we're sending the command to Serial (hardware UART).
  // Adjust if you want a dedicated port or a software serial.

  // Print a small debug banner:
  Serial.println();
  Serial.println("---------------------------------------------------------");
  Serial.print("[Relay] Sending command (");
  Serial.print(len);
  Serial.print(" bytes) => ");
  for (size_t i = 0; i < len; i++)
  {
    Serial.printf("0x%02X ", cmd[i]);
  }
  Serial.println();
  Serial.println("---------------------------------------------------------");

  Serial.write(cmd, len);
  Serial.println("[Relay] Command bytes written to Serial.");
  Serial.println("---------------------------------------------------------");
}