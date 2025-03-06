import os
import ssl
import paho.mqtt.client as mqtt

def init_mqtt_client():
    # Cria uma nova instância do cliente MQTT
    client = mqtt.Client()

    def on_connect(client, userdata, flags, rc):
        print("Connected with result code:", rc)

    client.on_connect = on_connect

    # Define os caminhos absolutos para os certificados
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    CA_CERT = os.path.join(BASE_DIR, "certs", "ca.crt")
    CLIENT_CERT = os.path.join(BASE_DIR, "certs", "client.crt")
    CLIENT_KEY = os.path.join(BASE_DIR, "certs", "client.key")

    # Cria um contexto SSL customizado e configura ALPN para a porta 443
    context = ssl.create_default_context()
    context.set_alpn_protocols(["x-amzn-mqtt-ca"])
    context.load_verify_locations(cafile=CA_CERT)
    context.load_cert_chain(certfile=CLIENT_CERT, keyfile=CLIENT_KEY)

    # Aplica o contexto customizado ao cliente MQTT
    client.tls_set_context(context)

    # Configura o broker e porta (usando a porta 443)
    MQTT_BROKER = "AWS_ENDPOINT"
    MQTT_PORT = 443
    MQTT_KEEPALIVE = 60

    # Conecta ao broker e inicia o loop de rede
    client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE)
    client.loop_start()

    return client

# Variável global para armazenar a instância do cliente MQTT por processo
mqtt_client_instance = None

def get_mqtt_client():
    global mqtt_client_instance
    if mqtt_client_instance is None:
        mqtt_client_instance = init_mqtt_client()
    return mqtt_client_instance

def publish_message(topic, message):
    """
    Publica uma mensagem no tópico MQTT especificado.
    Cada worker do Gunicorn terá sua própria instância de cliente,
    garantindo que o loop de rede esteja ativo durante toda a operação.
    """
    client = get_mqtt_client()
    result = client.publish(topic, message)
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"Published '{message}' to topic '{topic}'")
    else:
        print(f"Failed to publish message: {result.rc}")
    return result
