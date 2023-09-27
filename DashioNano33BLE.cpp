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

#ifdef ARDUINO_ARDUINO_NANO33BLE

#include "DashioNano33BLE.h"

const int BLE_MAX_SEND_MESSAGE_LENGTH = 100;

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : bleService(SERVICE_UUID),
            bleReadCharacteristic(CHARACTERISTIC_UUID, BLEWriteWithoutResponse, BLE_MAX_SEND_MESSAGE_LENGTH),
            bleWriteCharacteristic(CHARACTERISTIC_UUID, BLENotify, BLE_MAX_SEND_MESSAGE_LENGTH) {

    dashioDevice = _dashioDevice;
    printMessages = _printMessages;

    // Event driven reads.
    bleReadCharacteristic.setEventHandler(BLEWritten, onReadValueUpdate);

    // add the characteristic to the service
    bleService.addCharacteristic(bleReadCharacteristic);
    bleService.addCharacteristic(bleWriteCharacteristic);
}

MessageData DashioBLE::messageData(BLE_CONN);

void DashioBLE::onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic) {
    // central wrote new value to characteristic
    int dataLength = characteristic.valueLength();
    char* value = new char[dataLength + 1];  // one byte more, to save the '\0' character!
    characteristic.readValue(value, dataLength);
    value[dataLength] = '\0';  // make sure to null-terminate!
    messageData.processMessage(value);
    delete[] value;
}

void DashioBLE::setCallback(void (*processIncomingMessage)(MessageData *connection)) {
    processBLEmessageCallback = processIncomingMessage;
}

void DashioBLE::begin() {
    if (BLE.begin()) {
        // set advertised local name and service UUID:
        String localName = F("DashIO_");
        localName += dashioDevice->type;
        Serial.print(F("BLE local name: "));
        Serial.println(localName);
        BLE.setLocalName(localName.c_str());
        BLE.setDeviceName(localName.c_str());
        
        BLE.setConnectable(true);
        BLE.setAdvertisedService(bleService);

        // add service
        BLE.addService(bleService);

        // start advertising
        BLE.advertise();
    } else {
        Serial.println(F("Starting BLE failed"));
    }
}

void DashioBLE::sendMessage(const String& message) {
    if (BLE.connected()) {
        unsigned int maxMessageLength = BLE_MAX_SEND_MESSAGE_LENGTH;
        
        if (message.length() <= maxMessageLength) {
            bleWriteCharacteristic.writeValue(message.c_str());
        } else {
            int messageLength = message.length();
            unsigned int numFullStrings = messageLength / maxMessageLength;
    
            String subStr((char *)0);
            subStr.reserve(maxMessageLength);
            
            int start = 0;
            for (unsigned int i = 0; i < numFullStrings; i++) {
                subStr = message.substring(start, start + maxMessageLength);
                bleWriteCharacteristic.writeValue(subStr.c_str());
                start += maxMessageLength;
            }
            if (start < messageLength) {
                subStr = message.substring(start);
                bleWriteCharacteristic.writeValue(subStr.c_str());
            }
        }
    
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(message);
        }
    }
}

void DashioBLE::processConfig() {
    sendMessage(dashioDevice->getC64ConfigBaseMessage());
    
    int c64Length = strlen_P(dashioDevice->configC64Str);
    int length = 0;
    String message = "";
    for (int k = 0; k < c64Length; k++) {
        char myChar = pgm_read_byte_near(dashioDevice->configC64Str + k);
        
        message += myChar;
        length++;
        if (length == 100) {
            sendMessage(message);
            delay(200); // or BLE peripheral can't handle it
            message = "";
            length = 0;
        }
    }
    if (message.length() > 0) {
        sendMessage(message);
        delay(200); // or BLE peripheral can't handle it
    }

    sendMessage(String('\n'));
}

void DashioBLE::run() {
    if (BLE.connected()) {
        BLE.poll(); // Required for event handlers
        if (messageData.messageReceived) {
            messageData.messageReceived = false;
    
            if (printMessages) {
                Serial.println(messageData.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(messageData.control)));
            }
    
            switch (messageData.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            case config:
                processConfig();
                break;
            default:
                if (processBLEmessageCallback != NULL) {
                    processBLEmessageCallback(&messageData);
                }
                break;
            }
        }
        
        messageData.checkBuffer();
    }
}

bool DashioBLE::isConnected() {
    return BLE.connected();
}

void DashioBLE::end() {
    BLE.stopAdvertise();
    BLE.end();
}

String DashioBLE::macAddress() {
    return BLE.address();
}

#endif
