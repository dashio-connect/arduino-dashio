#if defined ESP32 || defined ESP8266

#ifndef DashioESP_h
#define DashioESP_h

#include "Arduino.h"
#include <arduino-timer.h>

#include <WiFiClientSecure.h> // Included in the espressif library
#include <MQTT.h>             // arduino-mqtt library created by Joël Gähwiler.
#ifdef ESP8266
    #include <ESP8266WiFi.h>  // Included in the 8266 Arduino library
    #include <ESP8266mDNS.h>  // Included in the 8266 Arduino library
#elif ESP32
    #include <BLEDevice.h>    // ESP32 BLE Arduino library by Neil Kolban. Included in Arduino IDE
    #include <ESPmDNS.h>      // Included in the espressif library
#endif

#include "DashIO.h"

extern const char *MQTT_SERVER;
extern const int MQTT_PORT;

#define SOFT_AP_PORT 55892

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ---------------------------------------- TCP ----------------------------------------

class DashioESP_TCP {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    MessageData data;
    uint16_t tcpPort = 5000;
    WiFiClient client;
    WiFiServer wifiServer;
    void (*processTCPmessageCallback)(MessageData *messageData);

public:
    DashioESP_TCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin();
    void begin(uint16_t _tcpPort);
    void sendMessage(const String& message);
    void setupmDNSservice();
    void startupServer();
    void checkForMessage();
    
    void mDNSend();
};

// ---------------------------------------- MQTT ---------------------------------------

class DashioESP_MQTT {
private:
    bool reboot = true;
    bool printMessages;
    DashioDevice *dashioDevice;
    static MessageData data;
    WiFiClientSecure wifiClient;
    MQTTClient mqttClient;
    int mqttConnectCount = 0;
    bool sendRebootAlarm;
    char *username;
    char *password;
    void (*processMQTTmessageCallback)(MessageData *messageData);

    static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length);
    void hostConnect();
    void setupLWT();

public:
    DashioESP_MQTT(DashioDevice *_dashioDevice, int bufferSize, bool _sendRebootAlarm, bool _printMessages = false);
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    void checkForMessage();
    void checkConnection();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin(char *_username, char *_password);
    void end();
};

// ---------------------------------------- BLE ----------------------------------------

#ifdef ESP32
class DashioESP_BLE {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    BLEServer *pServer;
    BLECharacteristic *pCharacteristic;

    void bleNotifyValue(const String& message);

public:
    MessageData data;
    void (*processBLEmessageCallback)(MessageData *messageData);

    DashioESP_BLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void checkForMessage();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin(bool secureBLE = false);
    String macAddress();
};
#endif

// ---------------------------------------- WiFi ---------------------------------------

class DashioESP_WiFi {
private:
    Timer<> timer;
    static bool oneSecond;
    int wifiConnectCount = 1;
    void (*wifiConnectCallback)(void);
    DashioESP_TCP *tcpConnection;
    DashioESP_MQTT *mqttConnection;

    static bool onTimerCallback(void *argument);

public:
    void attachConnection(DashioESP_TCP *_tcpConnection);
    void attachConnection(DashioESP_MQTT *_mqttConnection);
    void setOnConnectCallback(void (*connectCallback)(void));
    void begin(char *ssid, char *password);
    void run();
    void end();
    String macAddress();
    String ipAddress();
};

// --------------------------------------- Soft AP -------------------------------------

class DasgioESP_SoftAP {
private:
    DashioESP_TCP *tcpConnection;

public:
    bool begin(const String& password);
    void attachConnection(DashioESP_TCP *_tcpConnection);
    void end();
    bool isConnected();
    void run();
};

// -------------------------------------------------------------------------------------

#endif
#endif