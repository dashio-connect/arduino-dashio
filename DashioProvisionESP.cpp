/*
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

#if defined ESP32 || defined ESP8266

#include "DashioProvisionESP.h"

DashioProvision::DashioProvision(DashioDevice *_dashioDevice) {
    dashioDevice = _dashioDevice;
}

void DashioProvision::setup(int size) {
#ifdef ESP32
    if (!EEPROM.begin(PROVISIONING_EEPROM_SIZE + size)) {
        Serial.println(F("Failed to init EEPROM"));
    } else {
        eepromReady = true;
    }
#elif ESP8266
    EEPROM.begin(PROVISIONING_EEPROM_SIZE + size);
    eepromReady = true;
#endif
}

void DashioProvision::load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged)) {
    onProvisionCallback = _onProvisionCallback;
    update(defaultDeviceData);
    if (!eepromReady) {
        setup();
    }
    if (eepromReady) {
        load();
    }
}

void DashioProvision::processMessage(MessageData *messageData) {
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

void DashioProvision::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
}

void DashioProvision::save() {
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

void DashioProvision::load() {
#ifdef ESP32
/*???
    if (!EEPROM.begin(PROVISIONING_EEPROM_SIZE)) {
        Serial.println(F("Failed to init EEPROM"));
    } else {
*/
    DeviceData deviceDataRead;
    EEPROM.get(0, deviceDataRead);
    if (deviceDataRead.saved != 'Y') {
        save();
        Serial.println(F("User setup DEFAULTS used!"));
    } else {
        update(&deviceDataRead);
        Serial.println(F("User setup read from EEPROM"));
    }
//???    }
#elif ESP8266
//???    EEPROM.begin(PROVISIONING_EEPROM_SIZE);
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
