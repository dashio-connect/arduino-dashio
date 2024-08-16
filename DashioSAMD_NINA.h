/*
 DashioESP.h - Library for BLE, TCP & dash MQTT connectivity Arduino SAMD
 development boards.
 Created by C. Tuffnell, Dashio Connect Limited

 For more information, visit: https://dashio.io/guide-arduino-samd/

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

#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010 || defined ARDUINO_SAMD_MKRVIDOR4000
//#ifdef ARDUINO_ARCH_SAMD

#ifndef DashioSAMD_NINA_h
#define DashioSAMD_NINA_h

#include "Arduino.h"
#include <SPI.h>
#include <arduino-timer.h>

#include "Dashio.h"
//#include <WiFiNINA.h>
#include <WiFiNINA_Generic.h> // For mDNS
#include <ArduinoMqttClient.h>

#include <MDNS_Generic.hpp>
#define WIFI_NETWORK_WIFININA   true
#include <WiFiUdp_Generic.h> // mDNS for TCP

#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010

#include <ArduinoBLE.h>       // BLE

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#endif

// ---------------------------------------- TCP ----------------------------------------

class DashioTCP {
private:
    bool printMessages;
    DashioDevice *dashioDevice = nullptr;
    MessageData messageData;
    uint16_t tcpPort = 5650;
    WiFiClient client;
    WiFiServer wifiServer;

    WiFiUDP udp;
    MDNS mdns;

    void (*processTCPmessageCallback)(MessageData *connection) = nullptr;

public:
    DashioTCP(DashioDevice *_dashioDevice, bool _printMessages = false, uint16_t _tcpPort = 5650);
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void sendMessage(const String& message);
    void begin();
    void end();
    void run();
};

// ---------------------------------------- MQTT ---------------------------------------

class DashioMQTT {
private:
    bool reboot = true;
    bool printMessages;
    DashioDevice *dashioDevice = nullptr;
    static MessageData messageData;
    static WiFiSSLClient wifiClient;
    static MqttClient mqttClient;
    int mqttConnectCount = 0;
    bool sendRebootAlarm;
    char *username;
    char *password;
    void (*processMQTTmessageCallback)(MessageData *connection) = nullptr;

    DashStore *dashStore = nullptr;
    int dashStoreSize = 0;

    static void messageReceivedMQTTCallback(int messageSize);
    void hostConnect();

public:
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;

    DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages = false);
    void setup(char *_username, char *_password);
    void addDashStore(ControlType controlType, String controlID = "");
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    void sendWhoAnnounce();
    void checkConnection();
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void end();
};

// ---------------------------------------- WiFi ---------------------------------------

class DashioWiFi {
private:
    Timer<> timer;
    static bool oneSecond;
    int status = WL_IDLE_STATUS;
    IPAddress ipAddr = {0, 0, 0, 0};
    DashioTCP *tcpConnection = nullptr;
    DashioMQTT *mqttConnection = nullptr;

    static bool onTimerCallback(void *argument);

public:
    void attachConnection(DashioTCP *_tcpConnection);
    void attachConnection(DashioMQTT *_mqttConnection);
    bool begin(char *ssid, char *password, int maxRetries = 10000);
    bool run();
    void end();
    byte * macAddress();
    String ipAddress();
};

// ---------------------------------------- BLE ----------------------------------------
#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010

class DashioBLE {
private:
    bool printMessages;
    DashioDevice *dashioDevice = nullptr;
    static MessageData messageData;
    BLEService bleService;
    BLEStringCharacteristic bleReadCharacteristic;
    BLEStringCharacteristic bleWriteCharacteristic;

    static void onBLEConnected(BLEDevice central);
    static void onBLEDisconnected(BLEDevice central);
    static void onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic);

public:
    void (*processBLEmessageCallback)(MessageData *connection) = nullptr;

    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void begin();
    bool isConnected();
    void end();
    String macAddress();
};

#endif
// -------------------------------------------------------------------------------------
#endif
#endif
