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

void DashioProvision::load(void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged)) {
    onProvisionCallback = _onProvisionCallback;
    load();
}

void DashioProvision::load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged)) {
    onProvisionCallback = _onProvisionCallback;
    update(defaultDeviceData);
    load();
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
    case tcpSetup:
            setTCPport(messageData->idStr);
        break;
    }
}

void DashioProvision::setTCPport(const String &portStr) {
    int newTCPport = portStr.toInt();
    if (newTCPport > 1000) {
        tcpPort = newTCPport;
        save();
        Serial.print(F("TCP port changed to"));
        Serial.println(tcpPort);
    }
}

void DashioProvision::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
    tcpPort = deviceData->tcpPort;
}

void DashioProvision::save() {
    Serial.println(F("Saving user setup"));
    
    preferences.begin("dashio", false);
    preferences.putChar("stored", 'Y');
    preferences.putString("deviceName", dashioDevice->name);
    preferences.putString("wifiSSID", String(wifiSSID));
    preferences.putString("wifiPassword", String(wifiPassword));
    preferences.putString("dashUserName", String(dashUserName));
    preferences.putString("dashPassword", String(dashPassword));
    preferences.putInt("tcpPort", tcpPort);
    preferences.end();
}

void DashioProvision::load() {
    DeviceData deviceDataRead;
    preferences.begin("dashio", true);
    deviceDataRead.saved = preferences.getChar("stored", 'N');
    String str = preferences.getString("deviceName", "");
    str.toCharArray(deviceDataRead.deviceName, 32);
    if (str.length() > 0) {
        dashioDevice->name = str;
    }

    str = preferences.getString("wifiSSID", "");
    str.toCharArray(deviceDataRead.wifiSSID, 32);
    
    str = preferences.getString("wifiPassword", "");
    str.toCharArray(deviceDataRead.wifiPassword, 63);
    
    str = preferences.getString("dashUserName", "");
    str.toCharArray(deviceDataRead.dashUserName, 32);
    
    str = preferences.getString("dashPassword", "");
    str.toCharArray(deviceDataRead.dashPassword, 32);
    
    deviceDataRead.tcpPort = preferences.getInt("tcpPort", DEFAULT_TCP_PORT);
    preferences.end();

    if (deviceDataRead.saved != 'Y') {
        save();
        Serial.println(F("User setup DEFAULTS used!"));
    } else {
        update(&deviceDataRead);
        Serial.println(F("User setup read from EEPROM"));
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
    Serial.print(F("TCP port: "));
    Serial.println(deviceDataRead.tcpPort);
}

#endif
