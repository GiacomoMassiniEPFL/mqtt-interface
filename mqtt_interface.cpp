#include "mqtt_interface.h"
#include "nsapi_types.h"

nsapi_error_t MQTTInterface::init(uint8_t *local_mac, const char *local_addr, const char *mask, const char *gateway_addr, const char *broker_addr, int broker_port) {
    
    nsapi_error_t error_code;
    wiz_eth = new WIZnetInterface(WIZNET_MOSI, WIZNET_MISO, WIZNET_SCK, WIZNET_CS, WIZNET_RESET);
    wiz_eth->init(local_mac, local_addr, mask, gateway_addr);
    eth = wiz_eth;

    printf("Ethernet init and connect ok\n");
    printf("IP Address is %s\n", wiz_eth->get_ip_address());
    printf("IP Subnet is %s\n", wiz_eth->get_netmask());
    printf("IP Gateway is %s\n", wiz_eth->get_gateway());

    error_code = tcp_socket.open(eth);
    while (error_code != NSAPI_ERROR_OK) {
        printf("Error opening TCP socket, error %d, trying again\n", error_code);
        ThisThread::sleep_for(2s);
        error_code = tcp_socket.open(eth);
    }
    // tcp_socket.bind(MQTT_PORT);
    // tcp_socket.set_blocking(false);

    mqtt_endpoint = new SocketAddress(broker_addr, broker_port);
    printf("Broker address is %s\n", mqtt_endpoint->get_ip_address());
    printf("Broker port is %d\n", mqtt_endpoint->get_port());

    error_code = tcp_socket.connect(*mqtt_endpoint);
    while (error_code != NSAPI_ERROR_OK) {
        printf("Error connecting to TCP endpoint, error %d, trying again\n", error_code);
        ThisThread::sleep_for(2s);
        error_code = tcp_socket.connect(*mqtt_endpoint);
    }

    mqtt_client = new MQTTClient(&tcp_socket);

    return NSAPI_ERROR_OK;

}

nsapi_error_t MQTTInterface::connect() {
    nsapi_error_t error_code = MQTTInterface::connect(NULL, 0);
    while (error_code != NSAPI_ERROR_OK) {
        printf("Error connecting to MQTT broker, error %d, trying again\n", error_code);
        ThisThread::sleep_for(2s);
        error_code = MQTTInterface::connect(NULL, 0);
    }
    return NSAPI_ERROR_OK;
}

nsapi_error_t MQTTInterface::connect(char *clientID, short keepAliveInterval) {
    connect_data = MQTTPacket_connectData_initializer;
    connect_data.clientID.cstring = clientID;
    connect_data.keepAliveInterval = keepAliveInterval;
    return mqtt_client->connect(connect_data);
}

nsapi_error_t MQTTInterface::subscribe(const char *topic, uint8_t QoS) {
    return subscribe(topic, QoS, static_cast<MQTTClient::messageHandler>(MQTTInterface::callbackFunc));
}

nsapi_error_t MQTTInterface::subscribe(const char *topic, uint8_t QoS, void (*callbackFunc)(MQTT::MessageData &)) {
    nsapi_error_t error_code = mqtt_client->subscribe(topic, static_cast<MQTT::QoS>(QoS), callbackFunc);
    while (error_code != NSAPI_ERROR_OK) {
        printf("Error subscribing to topic, error %d, trying again\n", error_code);
        ThisThread::sleep_for(2s);
        error_code = mqtt_client->subscribe(topic, static_cast<MQTT::QoS>(QoS), callbackFunc);
    }
    return NSAPI_ERROR_OK;
}

nsapi_error_t MQTTInterface::yield(unsigned long timeout_ms) {
    return mqtt_client->yield(timeout_ms);
}

nsapi_error_t MQTTInterface::publish(string topic, string payload) {
    MQTT::Message msg;
    msg.qos = MQTT::QOS0;
    msg.retained = false;
    msg.dup = false;
    msg.payload = (void *) payload.c_str();
    msg.payloadlen = payload.length();
    return mqtt_client->publish(topic.c_str(), msg);
}

void MQTTInterface::callbackFunc(MQTT::MessageData &md) {
    string topicName(md.topicName.lenstring.data, md.topicName.lenstring.len);
    string payload((char *)md.message.payload, md.message.payloadlen);
    SimpleMessage newMessage = {topicName, payload};
    // SimpleMessage newMessage = { *new string(md.topicName.lenstring.data, md.topicName.lenstring.len), *new string((char *)md.message.payload, md.message.payloadlen) };
    if (messageQueue.size()>maxQueueSize) {
        messageQueue.pop();
        printf("Queue is full\n");
    }
    messageQueue.push(newMessage);
}

int MQTTInterface::maxQueueSize = 50;
std::queue<MQTTInterface::SimpleMessage> MQTTInterface::messageQueue;
