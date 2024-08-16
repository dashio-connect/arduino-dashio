/*
 DashioMKR1500.h - Library for dash MQTT connectivity for Arduino MKR1500
 development boards.
 Created by C. Tuffnell, Dashio Connect Limited
 
 For more information, visit: https://dashio.io/guide-arduino-mkrnb1500/

 MIT License

 Copyright (c) 2023 Craig Tuffnell, DashIO Connect Limited

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

#if defined ARDUINO_SAMD_MKRNB1500

#ifndef dashiomkr1500_h
#define dashiomkr1500_h

#include "Arduino.h"
#include <MKRNB.h>
#include <MQTT.h>              // arduino-mqtt library created by Joël Gähwiler.
#include "Dashio.h"
#include <arduino-timer.h>

// ---------------------------------------- LTE ----------------------------------------

class DashioLTE {
private:
    NBModem modem;
    NBScanner scannerNetworks;
    NB nbAccess;

    const char* pin = 0;
    const char* apn = "";
    const char* username = "";
    const char* password = "";
    
    bool printMessages;
    Timer<> timer;
    static bool oneSecond;

    int mqttRetry = 0;

    bool connectToCellNet();
    void resetCellModem();
    static bool onTimerCallback(void *argument);
    
    DashioMQTT *mqttConnection = nullptr;

public:
    bool cellConnected = false;
    bool modemIsReset = false;

    DashioLTE(bool _printMessages = false);
    void attachConnection(DashioMQTT *_mqttConnection);
    String getDeviceID();
    int getSignalStrength();
    void begin(const char* _pin = 0);
    void begin(const char* _pin, const char* _apn);
    void begin(const char* _pin, const char* _apn, const char* _username, const char* _password);
    void run();
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
    bool printMessages;
    static MessageData data;
    NBSSLClient nbsslCLient;
    MQTTClient mqttClient;
    int mqttConnectCount = 0;
    bool sendRebootAlarm;
    const char *username;
    const char *password;
    void (*processMQTTmessageCallback)(MessageData *messageData) = nullptr;
    void processConfig();

    static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length);
    void onConnected();
    void hostConnect();
    void setupLWT();

    DashStore *dashStore = nullptr;
    int dashStoreSize = 0;

public:
    DashioDevice *dashioDevice = nullptr;
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;
    bool passThrough = false;
    MQTTstate state = notReady;

    DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm = false, bool _printMessages = false);
    void setup(const char *_username, const char *_password);
    void addDashStore(ControlType controlType, String controlID = "");
    void sendMessage(const String& message, MQTTTopicType topic = data_topic);
    void sendAlarmMessage(const String& message);
    bool checkConnection();
    void run();
    void sendWhoAnnounce();
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void begin();
    void end();
};

# endif
# endif
