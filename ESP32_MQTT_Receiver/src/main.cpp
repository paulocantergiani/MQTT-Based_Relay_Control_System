#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Definição dos pinos da UART no ESP32
#define UART_BAUD 115200
#define TX_PIN 17 // GPIO17 (TX2)
#define RX_PIN 16 // GPIO16 (RX2)

// Comandos para os relés
const uint8_t A1_ON[] = {0xA0, 0x01, 0x01, 0xA2};
const uint8_t A1_OFF[] = {0xA0, 0x01, 0x00, 0xA1};
const uint8_t A2_ON[] = {0xA0, 0x02, 0x01, 0xA3};
const uint8_t A2_OFF[] = {0xA0, 0x02, 0x00, 0xA2};
const uint8_t A3_ON[] = {0xA0, 0x03, 0x01, 0xA4};
const uint8_t A3_OFF[] = {0xA0, 0x03, 0x00, 0xA3};
const uint8_t A4_ON[] = {0xA0, 0x04, 0x01, 0xA5};
const uint8_t A4_OFF[] = {0xA0, 0x04, 0x00, 0xA4};

// Credenciais WiFi
const char *ssid = "";
const char *password = "";

// Endpoint da AWS IoT
const char *awsEndpoint = "";

// Certificados e chave privada (armazenados em PROGMEM)
const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
  )EOF";
  
  const char client_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
  )EOF";
  
  const char client_key[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
-----END RSA PRIVATE KEY-----
  )EOF";

// Prototipagem das funções
void connectToWiFi();
void connectToAWS();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void sendCommand(const uint8_t *cmd, size_t len);

// Configuração do cliente seguro e MQTT
WiFiClientSecure net;
PubSubClient client(net);

void setup()
{
  Serial.begin(115200);
  Serial.println("\nIniciando ESP32 AWS IoT...");
  pinMode (2, OUTPUT);

  // Inicializa a comunicação UART
  Serial2.begin(UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

  // Desliga todos os relés inicialmente
  Serial.println("Inicializando relés...");
  sendCommand(A1_OFF, sizeof(A1_OFF));
  sendCommand(A2_OFF, sizeof(A2_OFF));
  sendCommand(A3_OFF, sizeof(A3_OFF));
  sendCommand(A4_OFF, sizeof(A4_OFF));

  // Conectar ao WiFi
  connectToWiFi();

  // Configura os certificados SSL/TLS
  net.setCACert(ca_cert);
  net.setCertificate(client_cert);
  net.setPrivateKey(client_key);

  // Configura o cliente MQTT
  client.setServer(awsEndpoint, 8883);
  client.setCallback(mqttCallback);

  // Conecta ao AWS IoT
  connectToAWS();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi desconectado! Reconectando...");
    connectToWiFi();
  }

  if (!client.connected())
  {
    Serial.println("MQTT desconectado! Reconectando...");
    connectToAWS();
  }

  client.loop();
  delay(100);
}

void connectToWiFi()
{
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (++attempts > 20)
    {
      Serial.println("\nFalha na conexão WiFi! Reiniciando...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void connectToAWS()
{
  Serial.println("Conectando ao AWS IoT...");

  while (!client.connected())
  {
    if (client.connect("esp32_relays_controller"))
    {
      Serial.println("Conectado ao AWS IoT!");
      client.subscribe("relays/relay1/control");
      client.subscribe("relays/relay2/control");
      client.subscribe("relays/relay3/control");
      client.subscribe("relays/relay4/control");
    }
    else
    {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void sendCommand(const uint8_t *cmd, size_t len)
{
  Serial2.write(cmd, len);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.println(message);

  // Lógica de controle dos relés
  if (message == "turn_on" && strcmp(topic, "relays/relay1/control") == 0)
  {
    Serial.println("Ativando relé 1...");
    sendCommand(A1_ON, sizeof(A1_ON));
    digitalWrite(2, HIGH);
  }
  else if (message == "turn_off" && strcmp(topic, "relays/relay1/control") == 0)
  {
    Serial.println("Desativando relé 1...");
    sendCommand(A1_OFF, sizeof(A1_OFF));
    digitalWrite(2, LOW);
  }

  if (message == "turn_on" && strcmp(topic, "relays/relay2/control") == 0)
  {
    Serial.println("Ativando relé 2...");
    sendCommand(A2_ON, sizeof(A2_ON));
  }
  else if (message == "turn_off" && strcmp(topic, "relays/relay2/control") == 0)
  {
    Serial.println("Desativando relé 2...");
    sendCommand(A2_OFF, sizeof(A2_OFF));
  }

  if (message == "turn_on" && strcmp(topic, "relays/relay3/control") == 0)
  {
    Serial.println("Ativando relé 3...");
    sendCommand(A3_ON, sizeof(A3_ON));
  }
  else if (message == "turn_off" && strcmp(topic, "relays/relay3/control") == 0)
  {
    Serial.println("Desativando relé 3...");
    sendCommand(A3_OFF, sizeof(A3_OFF));
  }

  if (message == "turn_on" && strcmp(topic, "relays/relay4/control") == 0)
  {
    Serial.println("Ativando relé 4...");
    sendCommand(A4_ON, sizeof(A4_ON));
  }
  else if (message == "turn_off" && strcmp(topic, "relays/relay4/control") == 0)
  {
    Serial.println("Desativando relé 4...");
    sendCommand(A4_OFF, sizeof(A4_OFF));
  }
}
