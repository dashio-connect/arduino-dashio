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
    
    DashioMQTT *mqttConnection;

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
    void (*processMQTTmessageCallback)(MessageData *messageData);
    void processConfig();

    static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length);
    void onConnected();
    void hostConnect();
    void setupLWT();

    DashStore *dashStore;
    int dashStoreSize = 0;

public:
    DashioDevice *dashioDevice;
    char *mqttHost = DASH_SERVER;
    uint16_t mqttPort = DASH_PORT;
    bool passThrough = false;
    MQTTstate state = notReady;

    DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm = false, bool _printMessages = false);
    void setup(const char *_username, const char *_password);
    void addDashStore(ControlType controlType, String controlID);
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
