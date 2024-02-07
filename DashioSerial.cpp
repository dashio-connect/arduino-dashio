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

#define C64_MAX_LENGHT 1000

DashSerial::DashSerial(DashDevice *_dashDevice, bool _printMessages) : data(SERIAL_CONN) {
    dashDevice = _dashDevice;
    printMessages = _printMessages;
}

void DashSerial::setCallbacksRxTx(void(*processIncomingMessage)(MessageData *messageData), void(*sendMessage)(const String& outgoingMessage)) {
    processRxMessageCallback = processIncomingMessage;
    txMessageCallback = sendMessage;
}

void DashSerial::processConfig() {
    txMessageCallback(dashDevice->getC64ConfigBaseMessage());
    
    int c64Length = strlen_P(dashDevice->configC64Str);
    int length = 0;
    String message = "";
    for (int k = 0; k < c64Length; k++) {
        char myChar = pgm_read_byte_near(dashDevice->configC64Str + k);

        message += myChar;
        length++;
        if (length == C64_MAX_LENGHT) {
            txMessageCallback(message);
            message = "";
            length = 0;
        }
    }
    message += "\n";
    txMessageCallback(message);
}

void DashSerial::actOnMessage() {
    //acts on whatever message is currently stored within data

    switch (data.control) {
        case who:
            responseMessage = dashDevice->getWhoMessage();
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(who)));
            }
            if (txMessageCallback != nullptr) {
                txMessageCallback(responseMessage);
            }
            break;
        case connect:
            responseMessage = dashDevice->getConnectMessage();
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(connect)));
            }
            if (txMessageCallback != nullptr) {
                txMessageCallback(responseMessage);
            }
            break;
        case config:
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(config)));
            }
            if (txMessageCallback != nullptr) {
                if (dashDevice->configC64Str != nullptr) {
                    processConfig();
                }
                txMessageCallback(responseMessage);
            }
            break;
        default:
            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashDevice->getControlTypeStr(data.control)));
            }
            if(processRxMessageCallback != nullptr) {
                processRxMessageCallback(&data);
            }
            break;
    }
}

void DashSerial::processChar(char chr) {
    if (data.processChar(chr)) {
        actOnMessage();
    }
}

void DashSerial::sendCtrl(ControlType controlType) {
    if (controlType == ctrl) {
        String message((char *)0);
        message.reserve(100);

        message += DELIM;
        message += CTRL;
        message += END_DELIM;

        if (txMessageCallback != nullptr) {
            txMessageCallback(message);
        }
    } else {
        sendCtrl(controlType, "");
    }
}

void DashSerial::sendCtrl(ControlType controlType, int value) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += CTRL;
    message += DELIM;
    message += dashDevice->getControlTypeStr(controlType);
    message += DELIM;
    message += value;
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
}

void DashSerial::sendCtrl(ControlType controlType, String value) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += CTRL;

    message += DELIM;
    if (controlType == ctrl) {
        message += value;
    } else {
        message += dashDevice->getControlTypeStr(controlType);
        if (value.length() > 0) {
            message += DELIM;
            message += value;
        }
    }
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
}

void DashSerial::sendCtrl(ControlType controlType, const String &value1, int value2) {
    if (controlType == config) {
        String message((char *)0);
        message.reserve(value1.length() + 100);

        message = DELIM;
        message += dashDevice->deviceID;
        message += DELIM;
        message += CTRL;

        message += DELIM;
        message += CFG;
        message += DELIM;
        message += value1;
        message += DELIM;
        message += String(value2);
        message += END_DELIM;

        if (txMessageCallback != nullptr) {
            txMessageCallback(message);
        }
    }
}

void DashSerial::sendCtrl(ControlType controlType, const String &value1, const String value2) {
    String message((char *)0);
    message.reserve(value1.length() + 100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += CTRL;
    message += DELIM;

    if (value1 == STORE_ENABLE) {
        message += value1;
        message += DELIM;
        message += dashDevice->getControlTypeStr(controlType);
    } else {
        message += dashDevice->getControlTypeStr(controlType);
        message += DELIM;
        message += value1;
    }

    message += DELIM;
    message += value2;
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
}

void DashSerial::sendName(const String &deviceName) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += NAME;
    message += DELIM;
    message += deviceName;
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
};

void DashSerial::sendWiFiCredentials(const String &SSID, const String &password, const String &countryCode) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += WIFI;
    message += DELIM;
    message += SSID;
    message += DELIM;
    message += password;
    message += DELIM;
    message += countryCode;
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
};

void DashSerial::sendTCPport(uint16_t port) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += TCP;
    message += DELIM;
    message += String(port);
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
};

void DashSerial::sendDashCredentials(const String &username, const String &password) {
    String message((char *)0);
    message.reserve(100);

    message = DELIM;
    message += dashDevice->deviceID;
    message += DELIM;
    message += DASHIO;
    message += DELIM;
    message += username;
    message += DELIM;
    message += password;
    message += END_DELIM;

    if (txMessageCallback != nullptr) {
        txMessageCallback(message);
    }
};
