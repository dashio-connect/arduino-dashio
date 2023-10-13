/*
 MIT License

 Copyright (c) 2023 Craig Tuffnell, DashIO Connect Limited

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

#include "DashioSerial.h"

DashSerial::DashSerial(DashDevice *_dashDevice, bool _printMessages) : data(SERIAL_CONN) {
    dashDevice = _dashDevice;
    printMessages = _printMessages;
}

void DashSerial::setCallback(void(*processIncomingMessage)(MessageData *messageData)) {
    processSerialmessageCallback = processIncomingMessage;
}

void DashSerial::setTransmit(void(*sendMessage)(const String& outgoingMessage)) {
    transmitMessage = sendMessage;
}

void DashSerial::actOnMessage() {
    //acts on whatever message is currently stored within data

    switch (data.control) {
        case who:
            responseMessage = dashDevice->getWhoMessage();
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(who)));
            }
            if (transmitMessage) {
                transmitMessage(responseMessage);
            }
            break;
        case connect:
            responseMessage = dashDevice->getConnectMessage();
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(connect)));
            }
            if (transmitMessage) {
                transmitMessage(responseMessage);
            }
            break;
        case config:
            if (dashDevice->configC64Str) {
                responseMessage = dashDevice->getC64ConfigMessage();
            }
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(config)));
            }
            if (transmitMessage) {
                transmitMessage(responseMessage);
            }
            break;
        default:
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(data.control)));
            }
            if(processSerialmessageCallback) {
                processSerialmessageCallback(&data);
            }
            break;
    }
}

void DashSerial::processChar(char chr) {
    if (data.processChar(chr)) {
        actOnMessage();
    }
}

void DashSerial::sendCtrlPing() {
    sendCtrllSetup("");
}

void DashSerial::sendCtrllSetup(const String &controlID, int value) {
    sendCtrllSetup(controlID, String(value));
}

void DashSerial::sendCtrllSetup(const String &controlID, const String& payload) {
    String message((char *)0);
    message.reserve(100);

    if (!controlID.isEmpty()) {
        message = DELIM;
        message += dashDevice->deviceID;
    }
    message += DELIM;
    message += CTRL;
    if (!controlID.isEmpty()) {
        message += DELIM;
        message += controlID;
    }
    if (!payload.isEmpty()) {
        message += DELIM;
        message += payload;
    }
    message += END_DELIM;

    transmitMessage(message);
}

void DashSerial::sendConfig(const String &cfgStr, int cfgRev) {
    String message((char *)0);
    message.reserve(cfgStr.length() + 100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += CFG;
    message += DELIM;
    message += cfgStr;
    message += DELIM;
    message += String(cfgRev);
    message += END_DELIM;

    transmitMessage(message);
}