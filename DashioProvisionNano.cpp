#ifdef ARDUINO_SAMD_NANO_33_IOT

#include "DashIOProvisionNano.h"

FlashStorage(flash_store, DeviceData);

DashioProvisionNano::DashioProvisionNano(DashioDevice *_dashioDevice, bool _eepromSave) {
    dashioDevice = _dashioDevice;
    eepromSave = _eepromSave;
}

void DashioProvisionNano::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
}

void DashioProvisionNano::save() {
    if (eepromSave) {
        Serial.println(F("User setup saving to EEPROM"));
        
        DeviceData deviceDataWrite;
        dashioDevice->name.toCharArray(deviceDataWrite.deviceName, dashioDevice->name.length() + 1);
        strcpy(deviceDataWrite.wifiSSID,     wifiSSID);
        strcpy(deviceDataWrite.wifiPassword, wifiPassword);
        strcpy(deviceDataWrite.dashUserName, dashUserName);
        strcpy(deviceDataWrite.dashPassword, dashPassword);

        deviceDataWrite.saved = 'Y';
        flash_store.write(deviceDataWrite);
    }
}

void DashioProvisionNano::load() {
    if (eepromSave) {
        DeviceData deviceDataRead;
        deviceDataRead = flash_store.read();

        if (deviceDataRead.saved != 'Y') {
            save();
            Serial.println(F("User setup DEFAULTS used!"));
        } else {
            update(&deviceDataRead);
            Serial.println(F("User setup read from Flash"));
        }
    }

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
