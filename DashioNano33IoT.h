#ifdef ARDUINO_SAMD_NANO_33_IOT

#ifndef Dashionano33IoT_h
#define Dashionano33IoT_h

#include "Arduino.h"
#include "DashIO.h"

#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoBLE.h>

extern const char *MQTT_SERVER;
extern const int MQTT_PORT;

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ---------------------------------------- WiFi ---------------------------------------

class DashioNano33_WiFi {
    private:
        int status = WL_IDLE_STATUS;

    public:
        void connect(char *ssid, char *password);
        byte * macAddress();
        void end();
};

// ---------------------------------------- TCP ----------------------------------------

class DashioNano33_TCP {
    private:
        bool printMessages;
        DashioDevice *dashioDevice;
        DashioConnection dashioConnection;
        uint16_t tcpPort = 5000;
        WiFiClient client;
        WiFiServer wifiServer;
        void (*processTCPmessageCallback)(DashioConnection *connection);
            
    public:
        DashioNano33_TCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
        void setup(void (*processIncomingMessage)(DashioConnection *connection));
        void sendMessage(const String& message);
        void begin();
        void checkForMessage();
//???        void end();
//???        void setupmDNSservice();
//???        void updatemDNS();

};

// ---------------------------------------- MQTT ---------------------------------------

class DashioNano_MQTT {
    private:
        bool reboot = true;
        bool printMessages;
        DashioDevice *dashioDevice;
        static DashioConnection dashioConnection;
        WiFiSSLClient wifiClient;
        PubSubClient mqttClient;
        int mqttConnectCount = 0;
        int bufferSize = 1024;
        bool sendRebootAlarm;
        char *username;
        char *password;
        void (*processMQTTmessageCallback)(DashioConnection *connection);

        static void messageReceivedMQTTCallback(char* topic, byte* payload, unsigned int length);
        void hostConnect();

    public:    
        DashioNano_MQTT(DashioDevice *_dashioDevice, int _bufferSize, bool _sendRebootAlarm, bool _printMessages = false);
        void sendMessage(const String& message, MQTTTopicType topic = data_topic);
        void sendAlarmMessage(const String& message);
        void checkForMessage();
        void checkConnection();
        void setup(char *_username, char *_password, void (*processIncomingMessage)(DashioConnection *connection));
        void end();
};

// ---------------------------------------- BLE ----------------------------------------

class DashioNano_BLE {
    private:
        bool printMessages;
        DashioDevice *dashioDevice;
        static DashioConnection dashioConnection;
        BLEService bleService;
        BLECharacteristic bleCharacteristic;

        static void onBLEConnected(BLEDevice central);
        static void onBLEDisconnected(BLEDevice central);
        static void onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic);

    public:    
        void (*processBLEmessageCallback)(DashioConnection *connection);

        DashioNano_BLE(DashioDevice *_dashioDevice, bool _printMessages = false);
        void sendMessage(const String& message);
        void checkForMessage();
        void setup(void (*processIncomingMessage)(DashioConnection *connection));
        void begin();
        bool connected();
        void end();
};

// -------------------------------------------------------------------------------------
#endif
#endif
