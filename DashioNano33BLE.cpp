#ifdef ARDUINO_ARDUINO_NANO33BLE

#include "DashioNano33BLE.h"

const int BLE_MAX_SEND_MESSAGE_LENGTH = 100;

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : bleService(SERVICE_UUID),
                                                                                   bleCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWriteWithoutResponse | BLENotify, BLE_MAX_SEND_MESSAGE_LENGTH, false) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;

    // Event driven reads.
    bleCharacteristic.setEventHandler(BLEWritten, onReadValueUpdate);

    // add the characteristic to the service
    bleService.addCharacteristic(bleCharacteristic);
}

MessageData DashioBLE::messageData(BLE_CONN);

void DashioBLE::onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic) {
    // central wrote new value to characteristic
    int dataLength = characteristic.valueLength();
    char data[dataLength];
    int finalLength = characteristic.readValue(data, dataLength);
    for (int i = 0; i < dataLength; i++) {
        if (messageData.processChar(data[i])) {
            messageData.messageReceived = true;
        }
    }
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
        int maxMessageLength = BLE_MAX_SEND_MESSAGE_LENGTH;
        
        if (message.length() <= maxMessageLength) {
            bleCharacteristic.writeValue(message.c_str());
        } else {
            int messageLength = message.length();
            int numFullStrings = messageLength / maxMessageLength;
    
            String subStr((char *)0);
            subStr.reserve(maxMessageLength);
            
            int start = 0;
            for (unsigned int i = 0; i < numFullStrings; i++) {
                subStr = message.substring(start, start + maxMessageLength);
                bleCharacteristic.writeValue(subStr.c_str());
                start += maxMessageLength;
            }
            if (start < messageLength) {
                subStr = message.substring(start);
                bleCharacteristic.writeValue(subStr.c_str());
            }
        }
    
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(message);
        }
    }
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
            default:
                if (processBLEmessageCallback != NULL) {
                    processBLEmessageCallback(&messageData);
                }
                break;
            }
        }
    }
}

bool DashioBLE::connected() {
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
