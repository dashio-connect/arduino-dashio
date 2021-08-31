#ifdef ARDUINO_SAMD_NANO_33_IOT

#ifndef Dashionano33IoT_h
#define Dashionano33IoT_h

#include "Arduino.h"
#include <SPI.h>
#include <arduino-timer.h>

#include "DashIO.h"
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoBLE.h>

extern const char *MQTT_SERVER;
extern const int MQTT_PORT;

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ---------------------------------------- TCP ----------------------------------------

class DashioTCP {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    MessageData messageData;
    uint16_t tcpPort = 5000;
    WiFiClient client;
    WiFiServer wifiServer;
    void (*processTCPmessageCallback)(MessageData *connection);

public:
    DashioTCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void sendMessage(const String& message);
    void begin();
    void end();
    void run();
//???    void setupmDNSservice();
//???    void updatemDNS();

};

// ---------------------------------------- MQTT ---------------------------------------

class DashioMQTT {
private:
    bool reboot = true;
    bool printMessages;
    DashioDevice *dashioDevice;
    static MessageData messageData;
    WiFiSSLClient wifiClient;
    PubSubClient mqttClient;
    int mqttConnectCount = 0;
    int bufferSize = 1024;
    bool sendRebootAlarm;
    char *username;
    char *password;
    void (*processMQTTmessageCallback)(MessageData *connection);

    static void messageReceivedMQTTCallback(char* topic, byte* payload, unsigned int length);
    void hostConnect();

public:
    DashioMQTT(DashioDevice *_dashioDevice, int _bufferSize, bool _sendRebootAlarm, bool _printMessages = false);
    void setup(char *_username, char *_password);
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    void checkConnection();
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void begin();
    void end();
};

// ---------------------------------------- BLE ----------------------------------------

class DashioBLE {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    static MessageData messageData;
    BLEService bleService;
    BLECharacteristic bleCharacteristic;

    static void onBLEConnected(BLEDevice central);
    static void onBLEDisconnected(BLEDevice central);
    static void onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic);

public:
    void (*processBLEmessageCallback)(MessageData *connection);

    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void begin();
    bool connected();
    void end();
    String macAddress();
};

// ---------------------------------------- WiFi ---------------------------------------

class DashioWiFi {
private:
    Timer<> timer;
    static bool oneSecond;
    int status = WL_IDLE_STATUS;
    IPAddress ipAddr = {0, 0, 0, 0};
    DashioTCP *tcpConnection;
    DashioMQTT *mqttConnection;

    static bool onTimerCallback(void *argument);

public:
    void attachConnection(DashioTCP *_tcpConnection);
    void attachConnection(DashioMQTT *_mqttConnection);
    bool begin(char *ssid, char *password, int maxRetries = 10000);
    void run();
    void end();
    byte * macAddress();
    String ipAddress();
};

// -------------------------------------------------------------------------------------
#endif
#endif
