/*
 MIT License

 Copyright (c) 2021 Craig Tuffnell, DashIO Connect Limited

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#if defined ESP32 || defined ESP8266

#ifndef DashioESP_h
#define DashioESP_h

#include "Arduino.h"
#include <arduino-timer.h>

#include <WiFiClientSecure.h>  // Included in the espressif library
#include <MQTT.h>              // arduino-mqtt library created by Joël Gähwiler.
#ifdef ESP8266
    #include <ESP8266WiFi.h>   // Included in the 8266 Arduino library
    #include <ESP8266mDNS.h>   // Included in the 8266 Arduino library
#elif ESP32
    #include <WiFi.h>
    #include <NimBLEDevice.h>  // ESP32 BLE Arduino library by Neil Kolban. Included in Arduino IDE
    #include <ESPmDNS.h>       // Included in the espressif library
#endif

#include "Dashio.h"

#define SOFT_AP_PORT 55892

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ---------------------------------------- TCP ----------------------------------------

struct TCPclient {
    WiFiClient client;
    MessageData data = MessageData(TCP_CONN);
};

class DashioTCP {
private:
    WiFiServer wifiServer;
    TCPclient *tcpClients;
    uint8_t maxTCPclients = 1;

    bool checkTCP(int index);
    void (*processTCPmessageCallback)(MessageData *messageData);
    void processConfig();

public:
    DashioDevice *dashioDevice;
    bool printMessages = false;
    uint16_t tcpPort = 5650;
    bool passThrough = false;
    uint8_t hasClient();

    DashioTCP(DashioDevice *_dashioDevice, bool _printMessages = false, uint16_t _tcpPort = 5650, uint8_t _maxTCPclients = 1);
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void setPort(uint16_t _tcpPort);
    void begin();
    void sendMessage(const String& message);
    void setupmDNSservice(const String& id);
    void startupServer();
    void run();
    
    void end();
};

// ---------------------------------------- MQTT ---------------------------------------
enum MQTTstate {
    notReady,
    disconnected,
    connecting,
    serverConnected,
    subscribed
};

class DashioMQTT {
private:
    bool reboot = true;
    static MessageData data;
    WiFiClientSecure wifiClient;
    MQTTClient mqttClient;
    unsigned long lastSentMessageTime;
    String mqttSendBuffer = ((char *)0);
    int mqttBuffersize = 0;
    int mqttConnectCount = 0;
    char *username;
    char *password;
    void (*processMQTTmessageCallback)(MessageData *messageData);
    void checkAndSendMQTTbuffer();
    void publishMessage(const String& message, MQTTTopicType topic);
    void processConfig();
#ifdef ESP32
    TaskHandle_t mqttConnectTask;
#endif

    static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length);
    void onConnected();
    void hostConnect();
    void setupLWT();
#ifdef ESP32
    static void checkConnectionTask(void * parameter);
#endif

    DashStore *dashStore;
    int dashStoreSize = 0;

public:
    DashioDevice *dashioDevice;
    bool printMessages = false;
    bool sendRebootAlarm = false;
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;
    bool wifiSetInsecure = true;
    bool passThrough = false;
    MQTTstate state = notReady;
    bool esp32_mqtt_blocking = true;

    DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm = false, bool _printMessages = false);
    DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages, int _mqttBufferSize);
    void setup(char *_username, char *_password);
    void addDashStore(ControlType controlType, String controlID = "");
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    void checkConnection();
    void run();
    void sendWhoAnnounce();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin();
    void end();
};

// ---------------------------------------- BLE ----------------------------------------

#ifdef ESP32
class DashioBLE {
private:
    bool secureBLE;
    BLEServer *pServer;
    BLECharacteristic *pCharacteristic;
    BLEAdvertising *pAdvertising;

    void bleNotifyValue(const String& message);
    void processConfig();

public:
    DashioDevice *dashioDevice;
    static bool printMessages;
    MessageData data;
    void (*processBLEmessageCallback)(MessageData *messageData);
    bool passThrough = false;

    static int connHandle;
    static bool authenticated;
    static bool authRequestConnect;

    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void end();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin(int passKey = 0);
    String macAddress();
    void advertise();
    bool isConnected();
};
#endif

// ---------------------------------------- WiFi ---------------------------------------

class DashioWiFi {
private:
    Timer<> timer;
    static bool oneSecond;
    int wifiConnectCount = 1;
    void (*wifiConnectCallback)(void);
    DashioTCP *tcpConnection;
    DashioMQTT *mqttConnection;

    static bool onTimerCallback(void *argument);

public:
    void attachConnection(DashioTCP *_tcpConnection);
    void attachConnection(DashioMQTT *_mqttConnection);
    void detachTcp();
    void detachMqtt();
    void setOnConnectCallback(void (*connectCallback)(void));
    void begin(char *ssid, char *password);
    void run();
    void end();
    String macAddress();
    String ipAddress();
};

// --------------------------------------- Soft AP -------------------------------------

class DashioSoftAP {
private:
    DashioTCP *tcpConnection;
    uint16_t originalTCPport = 5000;

public:
    bool begin(const String& password);
    void attachConnection(DashioTCP *_tcpConnection);
    void end();
    bool isConnected();
    void run();
};

// -------------------------------------------------------------------------------------

#endif
#endif
