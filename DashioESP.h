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

#define SOFT_AP_PORT 55892

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ---------------------------------------- TCP ----------------------------------------

class DashioTCP {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    MessageData data;
    WiFiClient client;
    WiFiServer wifiServer;
    void (*processTCPmessageCallback)(MessageData *messageData);

public:
    uint16_t tcpPort = 5000;

    DashioTCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
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

class DashioMQTT {
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
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;
    bool wifiSetInsecure = true;

    DashioMQTT(DashioDevice *_dashioDevice, int bufferSize, bool _sendRebootAlarm, bool _printMessages = false);
    void setup(char *_username, char *_password);
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    void run();
    void checkConnection();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin();
    void end();
};

// ---------------------------------------- BLE ----------------------------------------

#ifdef ESP32
class DashioBLE {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    BLEServer *pServer;
    BLECharacteristic *pCharacteristic;

    void bleNotifyValue(const String& message);

public:
    MessageData data;
    void (*processBLEmessageCallback)(MessageData *messageData);

    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin(bool secureBLE = false);
    String macAddress();
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
