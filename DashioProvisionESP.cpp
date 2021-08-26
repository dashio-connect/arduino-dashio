#if defined ESP32 || defined ESP8266

#include "DashIOProvisionESP.h"

DashioProvisionESP::DashioProvisionESP(DashioDevice *_dashioDevice) {
    dashioDevice = _dashioDevice;
}

void DashioProvisionESP::load(DeviceData *defaultDeviceData, void (*_onProvisionCallback)(ConnectionType connectionType, const String& message, bool commsChanged)) {
    onProvisionCallback = _onProvisionCallback;
    update(defaultDeviceData);
    load();
}

void DashioProvisionESP::processMessage(DashioConnection *connection) {
    switch (connection->control) {
    case deviceName:
        if (connection->idStr != "") {
            dashioDevice->name = connection->idStr;
            save();
            Serial.print(F("Updated Device Name: "));
            Serial.println(dashioDevice->name);
        }
        if (onProvisionCallback != NULL) {
            onProvisionCallback(connection->connectionType, dashioDevice->getDeviceNameMessage(), false);
        }
        break;
    case wifiSetup:
        connection->payloadStr.toCharArray(wifiSSID, connection->payloadStr.length() + 1);
        connection->payloadStr2.toCharArray(wifiPassword, connection->payloadStr2.length() + 1);
        save();
        Serial.print(F("Updated WIFI SSID: "));
        Serial.println(wifiSSID);
        Serial.print(F("Updated WIFI Password: "));
        Serial.println(wifiPassword);
        if (onProvisionCallback != NULL) {
            onProvisionCallback(connection->connectionType, dashioDevice->getWifiUpdateAckMessage(), true);
        }
        break;
    case dashioSetup:
        connection->idStr.toCharArray(dashUserName, connection->idStr.length() + 1);
        connection->payloadStr.toCharArray(dashPassword, connection->payloadStr.length() + 1);
        save();
        Serial.print(F("Updated Dash username: "));
        Serial.println(dashUserName);
        Serial.print(F("Updated Dash Password: "));
        Serial.println(dashPassword);
        if (onProvisionCallback != NULL) {
            onProvisionCallback(connection->connectionType, dashioDevice->getDashioUpdateAckMessage(), true);
        }
        break;
    }
}

void DashioProvisionESP::update(DeviceData *deviceData) {
    dashioDevice->name = String(deviceData->deviceName);
    strcpy(wifiSSID,     deviceData->wifiSSID);
    strcpy(wifiPassword, deviceData->wifiPassword);
    strcpy(dashUserName, deviceData->dashUserName);
    strcpy(dashPassword, deviceData->dashPassword);
}

void DashioProvisionESP::save() {
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

void DashioProvisionESP::load() {
#ifdef ESP32
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
#elif ESP8266
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
