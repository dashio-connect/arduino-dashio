#if defined ESP32 || defined ESP8266

#include "DashIOProvisionESP.h"

DashioProvisionESP::DashioProvisionESP(DashioDevice *_dashioDevice, bool _eepromSave) {
    dashioDevice = _dashioDevice;
    eepromSave = _eepromSave;
}

void DashioProvisionESP::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
}

void DashioProvisionESP::save() {
    if (eepromSave) {
        Serial.println(F("User setup saving to EEPROM"));
        
        DeviceData deviceDataWrite;
        dashioDevice->name.toCharArray(deviceDataWrite.deviceName, dashioDevice->name.length() + 1);
        strcpy(deviceDataWrite.wifiSSID,     wifiSSID);
        strcpy(deviceDataWrite.wifiPassword, wifiPassword);
        strcpy(deviceDataWrite.dashUserName, dashUserName);
        strcpy(deviceDataWrite.dashPassword, dashPassword);

        deviceDataWrite.saved = 'Y';
        EEPROM.put(0, deviceDataWrite);
        EEPROM.commit();
    }
}

void DashioProvisionESP::load() {
#ifdef ESP32
    if (eepromSave) {
        if (!EEPROM.begin(EEPROM_SIZE)) {
            Serial.println(F("Failed to init EEPROM"));
        } else {
            DeviceData deviceDataRead;
            EEPROM.get(0, deviceDataRead);
            if (deviceDataRead.saved != 'Y') {
                save();
                Serial.println(F("User setup DEFAULTS used!"));
            } else {
                update(&deviceDataRead);
                Serial.println(F("User setup read from EEPROM"));
            }
        }
    }
#elif  ESP8266
    EEPROM.begin(EEPROM_SIZE);
    DeviceData deviceDataRead;
    EEPROM.get(0, deviceDataRead);
    if (deviceDataRead.saved != 'Y') {
        save();
        Serial.println(F("User setup DEFAULTS used!"));
    } else {
        update(&deviceDataRead);
        Serial.println(F("User setup read from EEPROM"));
    }
#endif
    
    Serial.print(F("Device Name: "));
    Serial.println(dashioDevice->name);
    Serial.print(F("WiFi SSID: "));
    Serial.println(wifiSSID);
    Serial.print(F("WiFi password: "));
    Serial.println(wifiPassword);
    Serial.print(F("Dash userame: "));
    Serial.println(dashUserName);
    Serial.print(F("Dash password: "));
    Serial.println(dashPassword);
}

#endif
