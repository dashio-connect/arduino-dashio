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
        bool eepromSave = true;
        
        DashioDevice *dashioDevice;
        
        char wifiSSID[32];
        char wifiPassword[63];
        char dashUserName[32];
        char dashPassword[32];
    
        DashioProvisionESP(DashioDevice *_dashioDevice, bool _eepromSave);
        void update(DeviceData *deviceData);
        void save();
        void load();
};

#endif
