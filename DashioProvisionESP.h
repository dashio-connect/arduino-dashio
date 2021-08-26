#if defined ESP32 || defined ESP8266

#ifndef DashioProvisionESP_h
#define DashioProvisionESP_h

#include "Arduino.h"
#include "DashIO.h"
#include <EEPROM.h> // Arduino EEPROM library

#define EEPROM_SIZE 200

struct DeviceData {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];
    char saved;
};

class DashioProvisionESP {
public:
    DashioDevice *dashioDevice;
    
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];

    DashioProvisionESP(DashioDevice *_dashioDevice);
    void load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged));
    void processMessage(DashioConnection *connection);

private:
    void (*onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged);

    void load();
    void save();
    void update(DeviceData *deviceData);
};

#endif
#endif
