#ifdef ARDUINO_SAMD_NANO_33_IOT

#ifndef DashioProvisionNano_h
#define DashioProvisionNano_h

#include "Arduino.h"
#include "DashIO.h"
#include <FlashStorage.h>

typedef struct {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];
    char saved;
} DeviceData;

class DashioProvisionNano {
    public:
        bool eepromSave = true;
        DashioDevice *dashioDevice;
        
        char wifiSSID[32];
        char wifiPassword[63];
        char dashUserName[32];
        char dashPassword[32];
    
        DashioProvisionNano(DashioDevice *_dashioDevice, bool _eepromSave);
        void update(DeviceData *deviceData);
        void save();
        void load();
};

#endif
#endif
