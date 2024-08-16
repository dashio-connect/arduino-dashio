/*
 DashioESP.h - Library for BLE, TCP & dash MQTT connectivity for ESP32 & ESP8266
 development boards.
 Created by C. Tuffnell, Dashio Connect Limited
 
 For ESP32 vist: https://dashio.io/guide-arduino-esp32/
 and for ESP8266 visit: https://dashio.io/guide-arduino-esp8266/

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

#include <WiFiClientSecure.h>  // Included in the espressif library
#include <MQTT.h>              // arduino-mqtt library created by Joël Gähwiler.
#ifdef ESP8266
    #include <arduino-timer.h>
    #include <ESP8266WiFi.h>   // Included in the 8266 Arduino library
    #include <ESP8266mDNS.h>   // Included in the 8266 Arduino library
#elif ESP32
    #include <WiFi.h>
    #include <esp_wifi.h>
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
    TCPclient *tcpClients = nullptr;
    uint8_t maxTCPclients = 1;

    bool checkTCP(int index);
    void (*processTCPmessageCallback)(MessageData *messageData) = nullptr;
    void processConfig(uint16_t index);

public:
    DashioDevice *dashioDevice = nullptr;
    bool printMessages = false;
    uint16_t tcpPort = 5650;
    uint8_t hasClient();

    DashioTCP(DashioDevice *_dashioDevice, bool _printMessages = false, uint16_t _tcpPort = 5650, uint8_t _maxTCPclients = 1);
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void setPort(uint16_t _tcpPort);
    void begin();
    void sendMessage(const String& message, uint8_t index);
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
    char *username = nullptr;
    char *password = nullptr;
    void (*processMQTTmessageCallback)(MessageData *messageData) = nullptr;
    void checkAndSendMQTTbuffer();
    void publishMessage(const String& message, MQTTTopicType topic);
    void processConfig();
#ifdef ESP32
    TaskHandle_t mqttConnectTaskHandle; // Don't really need to keep this as it's not being used.
#endif

    static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length);
    void onConnected();
    void hostConnect();
    void setupLWT();
#ifdef ESP32
    static void checkConnectionTask(void * parameter);
#endif

    DashStore *dashStore = nullptr;
    int dashStoreSize = 0;

public:
    DashioDevice *dashioDevice = nullptr;
    bool printMessages = false;
    bool sendRebootAlarm = false;
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;
    bool wifiSetInsecure = true;
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
enum BLEauthState {
    BLE_NOT_AUTH,
    BLE_AUTH_REQ_CONN,
    BLE_AUTH_FAIL,
    BLE_AUTHENTICATED
};

struct BLEclientHolder {
    uint16_t connectionHandle = 65535;
    bool active = false;
    BLEauthState authState = BLE_NOT_AUTH;
};

class DashioBLE {
private:
    bool secureBLE = false;
    NimBLEServer *pServer = nullptr;
    NimBLECharacteristic *pCharacteristic = nullptr;
    NimBLEAdvertising *pAdvertising = nullptr;
    
    void initialiseClientHolders();
    void bleNotifyValue(const String& message);
    void processConfig();
    
public:
    DashioDevice *dashioDevice = nullptr;
    static bool printMessages;
    MessageData data;
    void (*processBLEmessageCallback)(MessageData *messageData) = nullptr;
    static uint32_t passKey;

    static BLEclientHolder *bleClients;
    static uint8_t maxBLEclients;
    
    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages, uint8_t _maxBLEclients);
    void sendMessage(const String& message);
    void run();
    void end();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin(uint32_t _passKey = 0);
    String macAddress();
    void advertise();
    bool isConnected();
    void setPassKey(uint32_t _passKey);

    static bool setConnectionActive(uint16_t conn_handle);
    static void setConnectionInactive(uint16_t conn_handle);
    static void setConnectionAuthState(uint16_t conn_handle, BLEauthState authState);
};
#endif

// ---------------------------------------- WiFi ---------------------------------------

class DashioWiFi {
private:
    DashioDevice *dashioDevice = nullptr;
    static bool oneSecond;
    int wifiConnectCount = 1;
    void (*wifiConnectCallback)(void) = nullptr; // Deprecated
    DashioTCP *tcpConnection = nullptr;
    DashioMQTT *mqttConnection = nullptr;

#ifdef ESP32
    TaskHandle_t wifiOneSecTaskHandle; // Don't really need to keep this as it's not being used.
    static void wifiOneSecondTask(void *parameter);
#elif ESP8266
    Timer<> timer;
    static bool onTimerCallback(void *argument);
#endif


public:
    DashioWiFi(DashioDevice *_dashioDevice = nullptr);

    void attachConnection(DashioTCP *_tcpConnection);
    void attachConnection(DashioMQTT *_mqttConnection);
    void detachTcp();
    void detachMqtt();
    void setOnConnectCallback(void (*connectCallback)(void)); // Deprecated
    void begin(char *ssid, char *password);
    void run();
    void end();
    String macAddress();
    String ipAddress();
};

// --------------------------------------- Soft AP -------------------------------------

class DashioSoftAP {
private:
    DashioTCP *tcpConnection = nullptr;
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
