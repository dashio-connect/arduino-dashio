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

#ifdef ARDUINO_AVR_UNO

#include "DashioBluno.h"

DashioBluno::DashioBluno(DashioDevice *_dashioDevice) : messageData(BLE_CONN) {
    dashioDevice = _dashioDevice;
}

void DashioBluno::sendMessage(const String& writeStr) {
    Serial.print(writeStr);
}

void DashioBluno::processConfig() {
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
            delay(100);
            message = "";
            length = 0;
        }
    }
    if (message.length() > 0) {
        sendMessage(message);
    }

    sendMessage(String('\n'));
}

void DashioBluno::run() {
    while(Serial.available()) {
        char data;
        data = (char)Serial.read();

        if (messageData.processChar(data)) {
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
    }
}

void DashioBluno::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processBLEmessageCallback = processIncomingMessage;
}

void DashioBluno::begin(bool useMacForDeviceID) {
    Serial.begin(115200); //initialise the Serial
    
    // Set peripheral name
    delay(350);
    Serial.print("+++"); // Enter AT mode
    delay(500);
    Serial.println("AT+NAME=DashIO_" + dashioDevice->type); // If the name has changed, requires a RESTART or power cycle
    delay(500);
    Serial.println("AT+NAME=?");
    delay(500);
    
    if (useMacForDeviceID) {
        // Get deviceID for mac address.
        while(Serial.available() > 0) {Serial.read();} // But first, clear the serial.
        Serial.println("AT+MAC=?");
        delay(1000); // Wait a while for reply
        dashioDevice->deviceID = "";
        while(Serial.available() > 0) {
            char c = Serial.read();
            if ((c == '\n') || (c == '\r')) { // Before the OK
                break;
            }
            dashioDevice->deviceID += c;
        }
        
        Serial.println("AT+EXIT");
        delay(1000);
        
        Serial.print(dashioDevice->getWhoMessage()); // In case WHO message is received when in AT mode
    }
}

#endif
