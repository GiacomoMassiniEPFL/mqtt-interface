#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include <string>
#include <queue>
#include "WIZnetInterface.h"
#include <MQTTClientMbedOs.h>

class MQTTInterface {

    NetworkInterface *eth;
    WIZnetInterface *wiz_eth;
    TCPSocket tcp_socket;
    SocketAddress *mqtt_endpoint;
    MQTTClient *mqtt_client;
    MQTTPacket_connectData connect_data;

  public:

    struct SimpleMessage {
        string topicName;
        string payload;
    };

    static int maxQueueSize;
    static std::queue<SimpleMessage> messageQueue;

    nsapi_error_t init(uint8_t *local_mac, const char *local_addr, const char *mask, const char *gateway_addr, const char *broker_addr, int broker_port);
    nsapi_error_t connect();
    nsapi_error_t connect(char *clientID, short keepAliveInterval);
    nsapi_error_t subscribe(const char *topic, uint8_t QoS);
    nsapi_error_t subscribe(const char *topic, uint8_t QoS, void (*callbackFunc)(MQTT::MessageData &));
    nsapi_error_t yield(unsigned long timeout_ms);
    nsapi_error_t publish(string topic, string payload);
    static void callbackFunc(MQTT::MessageData &md);

};

#endif