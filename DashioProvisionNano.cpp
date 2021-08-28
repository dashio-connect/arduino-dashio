#ifdef ARDUINO_SAMD_NANO_33_IOT

#include "DashIOProvisionNano.h"

FlashStorage(flash_store, DeviceData);

DashioProvisionNano::DashioProvisionNano(DashioDevice *_dashioDevice) {
    dashioDevice = _dashioDevice;
}

void DashioProvisionNano::load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged)) {
    onProvisionCallback = _onProvisionCallback;
    update(defaultDeviceData);
    load();
}

void DashioProvisionNano::processMessage(MessageData *messageData) {
    switch (messageData->control) {
    case deviceName:
        if (messageData->idStr != "") {
            dashioDevice->name = messageData->idStr;
            save();
            Serial.print(F("Updated Device Name: "));
            Serial.println(dashioDevice->name);
        }
        if (onProvisionCallback != NULL) {
            onProvisionCallback(messageData->connectionType, dashioDevice->getDeviceNameMessage(), false);
        }
        break;
    case wifiSetup:
        messageData->payloadStr.toCharArray(wifiSSID, messageData->payloadStr.length() + 1);
        messageData->payloadStr2.toCharArray(wifiPassword, messageData->payloadStr2.length() + 1);
        save();
        Serial.print(F("Updated WIFI SSID: "));
        Serial.println(wifiSSID);
        Serial.print(F("Updated WIFI Password: "));
        Serial.println(wifiPassword);
        if (onProvisionCallback != NULL) {
            onProvisionCallback(messageData->connectionType, dashioDevice->getWifiUpdateAckMessage(), true);
        }
        break;
    case dashioSetup:
        messageData->idStr.toCharArray(dashUserName, messageData->idStr.length() + 1);
        messageData->payloadStr.toCharArray(dashPassword, messageData->payloadStr.length() + 1);
        save();
        Serial.print(F("Updated Dash username: "));
        Serial.println(dashUserName);
        Serial.print(F("Updated Dash Password: "));
        Serial.println(dashPassword);
        if (onProvisionCallback != NULL) {
            onProvisionCallback(messageData->connectionType, dashioDevice->getDashioUpdateAckMessage(), true);
        }
        break;
    }
}

void DashioProvisionNano::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
}

void DashioProvisionNano::save() {
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

void DashioProvisionNano::load() {
    DeviceData deviceDataRead;
    deviceDataRead = flash_store.read();

    if (deviceDataRead.saved != 'Y') {
        save();
        Serial.println(F("User setup DEFAULTS used!"));
    } else {
        update(&deviceDataRead);
        Serial.println(F("User setup read from Flash"));
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
