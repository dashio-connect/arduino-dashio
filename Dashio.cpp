/*
 DashIO.cpp - Library for the DashIO comms protocol.
 Created by C. Tuffnell
 November 17, 2020
 
 MIT License

 Copyright (c) 2020 Craig Tuffnell, DashIO Connect Limited

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

#include "Dashio.h"
#include "DashioJSON.h"

// Control type IDs
#define CONNECT_ID "CONNECT"
#define WHO_ID "WHO"
#define CTRL_ID "CTRL"
#define STATUS_ID "STATUS"
#define CONFIG_ID "CFG"
#define CONFIG_C64 "C64"
#define CLOCK_ID "CLK"
// \t deviceID \t CLK \t srv_clk \t 1695347388 \n

#define DEVICE_ID "DVCE"
#define DEVICE_VIEW_ID "DVVW"
#define LABEL_ID "LBL"
#define BUTTON_ID "BTTN"
#define MENU_ID "MENU"
#define BUTTON_GROUP_ID "BTGP"
#define EVENT_LOG_ID "LOG"
#define SLIDER_ID "SLDR"
#define BAR_ID "BAR"
#define KNOB_ID "KNOB"
#define KNOB_DIAL_ID "KBDL"
#define TEXT_BOX_ID "TEXT"
#define TEXT_CAPTION_ID "TXTC"
#define SELECTOR_ID "SLCTR"
#define CHART_ID "CHRT"
#define TIME_GRAPH_ID "TGRPH"
#define DIRECTION_ID "DIR"
#define DIAL_ID "DIAL"
#define MAP_ID "MAP"
#define COLOR_ID "CLR"
#define AV_ID "AVD"
#define BASIC_CONFIG_ID "BAS"

#define DEVICE_NAME_ID "NAME"
#define WIFI_SETUP_ID "WIFI"
#define TCP_SETUP_ID "TCP"
#define DASHIO_SETUP_ID "DASHIO"
#define MQTT_SETUP_ID "MQTT"
#define RESET_DEVICE_ID "RST"

// Connection type IDs
#define MQTT_CONNECTION_ID "MQTT"
#define BLE_CONNECTION_ID "BLE"
#define TCP_CONNECTION_ID "TCP"

// Alarm
#define ALARM_ID "ALM"

// Store Enable
#define STORE_ENABLE_ID "STE"
#define STORE_AND_FORWARD_ID "SAF"

// Comms Controls
#define INIT_MODULE_ID "INIT"

// Button control states
#define BUTTON_ON "ON"
#define BUTTON_OFF "OFF"

// Graph/Chart control line types
#define LINE_ID "LINE"
#define BAR_GRAPH_ID "BAR"
#define SEGMENTED_BAR_ID "SEGBAR"
#define PEAK_BAR_ID "PEAKBAR"
#define BOOL_ID "BOOL"

// Graph/Chart control Y axis select options
#define LEFT_ID "LEFT"
#define RIGHT_ID "RIGHT"

// MQTT topic tips
#define DATA_TOPIC_TIP     "data"
#define CONTROL_TOPIC_TIP  "control"
#define ALARM_TOPIC_TIP    "alarm"
#define ANNOUNCE_TOPIC_TIP "announce"
#define WILL_TOPIC_TIP     "data"

// MQTT basic messages
#define MQTT_ONLINE_MSSG  "\tONLINE\n"
#define MQTT_OFFLINE_MSSG "\tOFFLINE\n"

#define MAX_STRING_LEN 64
#define MAX_DEVICE_NAME_LEN 32
#define MAX_DEVICE_TYPE_LEN 32

// Server types
#define SERVER_CLK = "srv_clk"

char DASH_SERVER[] = "dash.dashio.io";

String formatFloat(float value) {
    if (value == INVALID_FLOAT_VALUE) {
        return "nan";
    } else if (abs(value) < SMALLEST_FLOAT_VALUE) {
        return "0";
    }
    
    char buffer[16];
#ifdef ARDUINO_ARCH_AVR
    dtostrf(value, 5, 2, buffer);
    return buffer;
#else
    if ((abs(value) < 1.0) || (abs(value) >= 100000)){
        sprintf(buffer, "%5.2e", value);
    } else {
        sprintf(buffer, "%5.2f", value);
    }
    return buffer;
#endif
}

String formatInt(int value) {
    if (value == INVALID_INT_VALUE) {
        return "nan";
    } else {
        return String(value);
    }
}

MessageData::MessageData(ConnectionType connType, int _bufferLength) {
    bufferLength = _bufferLength;
    buffer = new char [bufferLength];
    
    deviceID.reserve(MAX_STRING_LEN);
    idStr.reserve(MAX_STRING_LEN);
    payloadStr.reserve(MAX_STRING_LEN);
    
    connectionType = connType;
};

void MessageData::loadBuffer(const String& message, uint16_t _connectionHandle) {
    // Connection Handle is stored as the first bytes (of PREFIX_LEN) in the buffer message
    uint prefixLen = sizeof(uint16_t);
    char connectionHandleChrs[prefixLen];
    memcpy(connectionHandleChrs, &_connectionHandle, prefixLen);

    int messageLength = message.length() + prefixLen;
    int avail = 0;
    if (bufferLength > 0) {
        if (bufferWritePtr >= bufferReadPtr) {
            avail = bufferLength - (bufferWritePtr - bufferReadPtr);
        } else {
            avail = bufferReadPtr - bufferWritePtr;
        }
    }
    
    if (avail < messageLength) {
#ifdef ESP32
        ESP_LOGI(DTAG, "%s Buffer overflow - can't process message: : %s", getConnectionTypeStr(), message);
#else
        Serial.print(getConnectionTypeStr() + " ");
        Serial.print(F("Buffer overflow - can't process message: "));
        Serial.println(message);
#endif
    } else {
        for (int i = 0; i < messageLength; i++) {
            if (i < prefixLen) {
                buffer[bufferWritePtr] = connectionHandleChrs[i];
            } else {
                buffer[bufferWritePtr] = message[i - prefixLen];
            }
            bufferWritePtr++;
            if (bufferWritePtr >= bufferLength) {
                bufferWritePtr = 0;
            }
        }
    }
}

void MessageData::checkBuffer() {
    if (!messageReceived) { // wait until last message processed
        if (bufferWritePtr != bufferReadPtr) {
            uint prefixLen = sizeof(uint16_t);
            char prefixChrs[prefixLen];
            bool unloadedMessage = false;
            int count = 0;
            while (bufferReadPtr != bufferWritePtr) { // read pointer has caught up to write pointer, therefore, must be the end
                if (count++ > bufferLength) { // can't find valid message, so quit
                    unloadedMessage = false;
                    break;
                }

                char chr = buffer[bufferReadPtr];

                bufferReadPtr++;
                if (bufferReadPtr >= bufferLength) {
                    bufferReadPtr = 0;
                }

                if (count <= prefixLen) {
                    prefixChrs[count - 1] = chr; // Retrieve Connection Handle bytes
                } else {
                    if (processChar(chr)) {
                        memcpy(&connectionHandle, prefixChrs, prefixLen);
                        unloadedMessage = true;
                        break;
                    }
                }
            }

            messageReceived = unloadedMessage;
        }
    }
}

void MessageData::processMessage(const String& message, uint16_t _connectionHandle) {
    if (message.length() > 0) {
        if (bufferLength > 0) { // Storing to buffer allows for concatenated dash messages
            loadBuffer(message, _connectionHandle);
        } else {
            for (unsigned int i = 0; i < message.length(); i++) {
                char chr = message[i];
                if (processChar(chr)) {
                    connectionHandle = _connectionHandle;
                    messageReceived = true;
                }
            }
        }
    }
}

bool MessageData::processChar(char chr) {
    bool messageEnd = false;
    if ((chr == DELIM) || (chr == END_DELIM)) {
        if ((readStr.length() > 0) || (segmentCount == 1)) { // segmentCount == 1 allows for empty second field ??? maybe should be 2 for empty third field now that we've added deviceID at the front
            switch (segmentCount) {
            case 0:
                if (checkPrefix && readStr == BLE_CONNECTION_ID) {
                    connectionType = BLE_CONN;
                    segmentCount = -1;
                } else if (checkPrefix && readStr == TCP_CONNECTION_ID) {
                    connectionType = TCP_CONN;
                    segmentCount = -1;
                } else if (checkPrefix && readStr == MQTT_CONNECTION_ID) {
                    connectionType = MQTT_CONN;
                    segmentCount = -1;
                } else if (readStr == WHO_ID) {
                    deviceID = "---";
                    control = who;
                } else {
                    deviceID = readStr;
                    control = unknown;
                }
                      
                idStr = "";
                payloadStr = "";
                payloadStr2 = "";
                break;
            case 1:
                if (readStr == WHO_ID) {
                    control = who;
                } else if (readStr == CTRL_ID) {
                    control = ctrl;
                } else if (readStr == CONNECT_ID) {
                    control = connect;
                } else if (readStr == CLOCK_ID) {
                    control = dashClock;
                } else if (readStr == STATUS_ID) {
                    control = status;
                } else if (readStr == CONFIG_ID) {
                    control = config;
                } else if (readStr == BUTTON_ID) {
                    control = button;
                } else if (readStr == SLIDER_ID) {
                    control = slider;
                } else if (readStr == KNOB_ID) {
                    control = knob;
                } else if (readStr == TEXT_BOX_ID) {
                    control = textBox;
                } else if (readStr == TIME_GRAPH_ID) {
                    control = timeGraph;
                } else if (readStr == MENU_ID) {
                    control = menu;
                } else if (readStr == BUTTON_GROUP_ID) {
                    control = buttonGroup;
                } else if (readStr == EVENT_LOG_ID) {
                    control = eventLog;
                } else if (readStr == SELECTOR_ID) {
                    control = selector;
                } else if (readStr == COLOR_ID) {
                    control = colorPicker;
                } else if (readStr == DEVICE_NAME_ID) {
                    control = deviceName;
                } else if (readStr == WIFI_SETUP_ID) {
                    control = wifiSetup;
                } else if (readStr == TCP_SETUP_ID) {
                    control = tcpSetup;
                } else if (readStr == DASHIO_SETUP_ID) {
                    control = dashioSetup;
                } else if (readStr == MQTT_SETUP_ID) {
                    control = mqttSetup;
                } else if (readStr == RESET_DEVICE_ID) {
                    control = resetDevice;
                } else {
                    control = unknown;
                    segmentCount = -1;
                }
                break;
            case 2:
                idStr = readStr;
                break;
            case 3:
                payloadStr = readStr;
                break;
            case 4:
                payloadStr2 = readStr;
                break;
            default:
                    segmentCount = 0;
            }

            if (segmentCount >= 0) {
                segmentCount++;
                if (chr == END_DELIM) { // End of message, so process message
                    messageEnd = true;
                    segmentCount = -1; // Wait for next start of message
                }
            }
        } else {
            segmentCount = 0; // Must have no data before DELIM or a DELIM + DELIM, so must be start of message
        }
        readStr = "";
    } else {
        readStr += chr;
    }
    return messageEnd;
}

String MessageData::getMessageGeneric(const String& controlStr, bool connectionPrefix) {
    String message((char *)0);
    message.reserve(100);
    
    if (connectionPrefix) {
        message += String(DELIM);
        message += getConnectionTypeStr();
    }
    
    if (!(controlStr == String("WHO"))){
        message += String(DELIM);
        message += deviceID;
    }
    if (controlStr.length() > 0) {
        message += String(DELIM);
        message += controlStr;
    }
    if (idStr.length() > 0) {
        message += String(DELIM);
        message += idStr;
    }
    if (payloadStr.length() > 0) {
        message += String(DELIM);
        message += payloadStr;
    }
    if (payloadStr2.length() > 0) {
        message += String(DELIM);
        message += payloadStr2;
    }
    message += String(END_DELIM);

    return message;
}

String MessageData::getReceivedMessageForPrint(const String& controlStr) {
    String message((char *)0);
    message.reserve(100);

    message += F("**** ");
    message += getConnectionTypeStr();
    message += F(" Received ****\n");
    message += getMessageGeneric(controlStr);
    return message;
}

String MessageData::getTransmittedMessageForPrint(const String& controlStr) {
    String message((char *)0);
    message.reserve(100);

    message += F("**** ");
    message += getConnectionTypeStr();
    message += F(" Sent ****\n");
    message += getMessageGeneric(controlStr);
    return message;
}

String MessageData::getConnectionTypeStr() {
    switch (connectionType) {
        case BLE_CONN:
            return "BLE";
        case TCP_CONN:
            return "TCP";
        case MQTT_CONN:
            return "MQTT";
        case SERIAL_CONN:
            return "SERIAL";
        case ALL_CONN:
            return "ALL";
    }
    return "";
}

/* --------------- */
DashioDevice::DashioDevice(const String& _deviceType, const char *_configC64Str, unsigned int _cfgRevision) {
    type.reserve(MAX_DEVICE_TYPE_LEN);
    type = _deviceType;
    configC64Str = _configC64Str;
    cfgRevision = _cfgRevision;

    name.reserve(MAX_DEVICE_NAME_LEN);
    deviceID.reserve(MAX_STRING_LEN);
}

void DashioDevice::setup(const String& deviceIdentifier) {
    if (name == "") {
        name = F(DEFAULT_DEVICE_NAME);
    }
    deviceID = deviceIdentifier;
}

void DashioDevice::setup(const String& deviceIdentifier, const String& _deviceName) {
    name = _deviceName;
    deviceID = deviceIdentifier;
}

void DashioDevice::setup(uint8_t m_address[6], const String& _deviceName) {
    name = _deviceName;
    DashioDevice::setup(m_address);
}

void DashioDevice::setup(uint8_t m_address[6]) {
    char buffer[20];
    String macStr((char *)0);
    macStr.reserve(20);

    sprintf(buffer, "%x", m_address[0]);
    if (m_address[0] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[1]);
    if (m_address[1] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[2]);
    if (m_address[2] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[3]);
    if (m_address[3] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[4]);
    if (m_address[4] < 16) {
        macStr += "0";
    }
    macStr += buffer;
    sprintf(buffer, "%x", m_address[5]);
    if (m_address[5] < 16) {
        macStr += "0";
    }
    macStr += buffer;

    deviceID = macStr.c_str();
}

void DashioDevice::setStatusCallback(void (*_statusCallback)(StatusCode statusCode)) {
    statusCallback = _statusCallback;
}

void DashioDevice::onStatusCallback(StatusCode statusCode) {
    if (statusCallback != nullptr) {
        statusCallback(statusCode);
    }
}

void DashioDevice::appendDelimitedStr(String *str, const String& addStr) {
    String message = *str;
    *str += String(DELIM);
    *str += addStr;
}

String DashioDevice::getOnlineMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += MQTT_ONLINE_MSSG;
    return message;
}

String DashioDevice::getOfflineMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += MQTT_OFFLINE_MSSG;
    return message;
}

String DashioDevice::getDataStoreEnableMessage(DashStore dashStore) {
    String message = getControlBaseMessage(STORE_ENABLE_ID, getControlTypeStr(dashStore.controlType));
    message += dashStore.controlID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getWhoMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += WHO_ID;
    message += String(DELIM);
    message += type;
    message += String(DELIM);
    message += name;
    message += String(DELIM);
    message += String(cfgRevision);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getConnectMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONNECT_ID;
    message += String(END_DELIM);
    return  message;
}

String DashioDevice::getClockMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CLOCK_ID;
    message += String(END_DELIM);
    return  message;
}

String DashioDevice::getDeviceNameMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += DEVICE_NAME_ID;
    message += String(DELIM);
    message += name;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getWifiUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += WIFI_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getTCPUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += TCP_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDashioUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += DASHIO_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getMQTTUpdateAckMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += MQTT_SETUP_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getResetDeviceMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += RESET_DEVICE_ID;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getAlarmMessage(const String& controlID, const String& title, const String& description) {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += ALARM_ID;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
    message += title;
    message += String(DELIM);
    message += description;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getAlarmMessage(Notification alarm) {
    return getAlarmMessage(alarm.identifier, alarm.title, alarm.description);
}

String DashioDevice::getControlBaseMessage(const String& controlType, const String& controlID) {
    String message = "";
    addControlBaseMessage(message, controlType, controlID);
    return message;
}

void DashioDevice::addControlBaseMessage(String& message, const String& controlType, const String& controlID) {
    message += String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += controlType;
    message += String(DELIM);
    message += controlID;
    message += String(DELIM);
}

String DashioDevice::getButtonMessage(const String& controlID) {
    String message = getControlBaseMessage(BUTTON_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getButtonMessage(const String& controlID, bool on, const String& iconName, const String& text) {
    String message = getControlBaseMessage(BUTTON_ID, controlID);
    if (on) {
        message += BUTTON_ON;
    } else {
        message += BUTTON_OFF;
    }
    if (text != "") {
        message += String(DELIM);
        message += iconName;
        message += String(DELIM);
        message += text;
    } else {
        if (iconName != "") {
            message += String(DELIM);
            message += iconName;
        }
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getTextBoxMessage(const String& controlID, const String& text, const String& color) {
    String message = getControlBaseMessage(TEXT_BOX_ID, controlID);
    message += text;
    if (color != "") {
        message += String(DELIM);
        message += color;
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getTextBoxCaptionMessage(const String& controlID, const String& text, const String& color) {
    String message = getControlBaseMessage(TEXT_CAPTION_ID, controlID);
    message += text;
    if (color != "") {
        message += String(DELIM);
        message += color;
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSelectorMessage(const String& controlID) {
    String message = getControlBaseMessage(SELECTOR_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSelectorMessage(const String& controlID, int index) {
    String message = getControlBaseMessage(SELECTOR_ID, controlID);
    message += String(index);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSelectorMessage(const String& controlID, int index, String selectionItems[], int numItems) {
    String message = getControlBaseMessage(SELECTOR_ID, controlID);
    message += String(index);
    for (int i = 0; i < numItems; i++) {
        message += String(DELIM);
        message += selectionItems[i];
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSelectorMessage(const String& controlID, int index, const String& selectionStr) {
    String message = getControlBaseMessage(SELECTOR_ID, controlID);
    message += String(index);
    message += selectionStr;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSliderMessage(const String& controlID, int value) {
    String message = getControlBaseMessage(SLIDER_ID, controlID);
    message += formatInt(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSliderMessage(const String& controlID, float value) {
    String message = getControlBaseMessage(SLIDER_ID, controlID);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSliderMessage(const String& controlID) {
    String message = getControlBaseMessage(SLIDER_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSingleBarMessage(const String& controlID, int value) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    message += formatInt(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSingleBarMessage(const String& controlID, float value) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getSingleBarMessage(const String& controlID) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDoubleBarMessage(const String& controlID, int value1, int value2) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    int barValues[2];
    barValues[0] = value1;
    barValues[1] = value2;
    addIntArray(message, barValues, 2);
    return message;
}

String DashioDevice::getDoubleBarMessage(const String& controlID, float value1, float value2) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    float barValues[2];
    barValues[0] = value1;
    barValues[1] = value2;
    addFloatArray(message, barValues, 2);
    return message;
}

String DashioDevice::getDoubleBarMessage(const String& controlID) {
    String message = getControlBaseMessage(BAR_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(DELIM);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobMessage(const String& controlID, int value) {
    String message = getControlBaseMessage(KNOB_ID, controlID);
    message += formatInt(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobMessage(const String& controlID, float value) {
    String message = getControlBaseMessage(KNOB_ID, controlID);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobMessage(const String& controlID) {
    String message = getControlBaseMessage(KNOB_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobDialMessage(const String& controlID, int value) {
    String message = getControlBaseMessage(KNOB_DIAL_ID, controlID);
    message += formatInt(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobDialMessage(const String& controlID, float value) {
    String message = getControlBaseMessage(KNOB_DIAL_ID, controlID);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getKnobDialMessage(const String& controlID) {
    String message = getControlBaseMessage(KNOB_DIAL_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDialMessage(const String& controlID, int value) {
    String message = getControlBaseMessage(DIAL_ID, controlID);
    message += formatInt(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDialMessage(const String& controlID, float value) {
    String message = getControlBaseMessage(DIAL_ID, controlID);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDialMessage(const String& controlID) {
    String message = getControlBaseMessage(DIAL_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDirectionMessage(const String& controlID, int direction, float speed) {
    String message = getControlBaseMessage(DIRECTION_ID, controlID);
    message += formatInt(direction);
    if (speed >= 0) {
        message += String(DELIM);
        message += formatFloat(speed);
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDirectionMessage(const String& controlID, float direction, float speed) {
    String message = getControlBaseMessage(DIRECTION_ID, controlID);
    message += formatFloat(direction);
    if (speed >= 0) {
        message += String(DELIM);
        message += formatFloat(speed);
    }
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getDirectionMessage(const String& controlID) {
    String message = getControlBaseMessage(DIRECTION_ID, controlID);
    message += NOT_AVAILABLE;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getMapWaypointMessage(const String& controlID, const String& trackID, const String& latitude, const String& longitude) {
    String message = getControlBaseMessage(MAP_ID, controlID);
    message += trackID;
    message += String(DELIM);
    message += latitude;
    message += ",";
    message += longitude;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getMapWaypointMessage(const String& controlID, const String& trackID, float latitude, float longitude) {
    char latLonBuffer[16];
    String message = getControlBaseMessage(MAP_ID, controlID);
    message += trackID;
    message += String(DELIM);
    sprintf(latLonBuffer, "%f", latitude);
    message += latLonBuffer;
    message += ",";
    sprintf(latLonBuffer, "%f", longitude);
    message += latLonBuffer;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getMapTrackMessage(const String& controlID, const String& trackID, const String& text, const String& colour, Waypoint waypoints[], int numWaypoints) {
    String message = getControlBaseMessage(MAP_ID, controlID);
    message += dashboardID;
    message += String(DELIM);
    message += trackID;
    message += String(DELIM);
    message += text;
    message += String(DELIM);
    message += colour;
    
    for (int i = 0; i < numWaypoints; i++) {
        message += String(DELIM);
        addWaypointJSON(message, waypoints[i]);
    }
    
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getColorMessage(const String& controlID, const String& color) {
    String message = getControlBaseMessage(COLOR_ID, controlID);
    message += color;
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getAudioVisualMessage(const String& controlID, const String& url) {
    String message = getControlBaseMessage(AV_ID, controlID);
    message += url;
    message += String(END_DELIM);
    return message;
}

void DashioDevice::addEventLogMessage(String& message, const String& controlID, const String& timeStr, const String& color, String text[], int numTextRows) {
    addControlBaseMessage(message, EVENT_LOG_ID, controlID);
    message += timeStr;
    message += String(DELIM);
    message += color;
    for (int i = 0; i < numTextRows; i++) {
        message += String(DELIM);
        message += text[i];
    }
    message += String(END_DELIM);
}

void DashioDevice::addEventLogMessage(String& message, const String& controlID, const String& color, String text[], int numTextRows) {
    return addEventLogMessage(message, controlID, "", color, text, numTextRows);
}

void DashioDevice::addEventLogMessage(String& message, const String& controlID, Event events[], int numEvents) {
    addControlBaseMessage(message, EVENT_LOG_ID, controlID);
    message += dashboardID;
    message += String(DELIM);

    for (int i = 0; i < numEvents; i++) {
        addEventJSON(message, events[i]);
        if (i < numEvents - 1) { // because getControlBaseMessage ends in a DELIM
            message += String(DELIM);
        }
    }
    message += String(END_DELIM);
}

String DashioDevice::getC64ConfigBaseMessage() {
    String message = String(DELIM);
    message += deviceID;
    message += String(DELIM);
    message += CONFIG_ID;
    message += String(DELIM);
    message += dashboardID;
    message += String(DELIM);
    message += CONFIG_C64;
    message += String(DELIM);
    return message;
}

String DashioDevice::getC64ConfigMessage() {
    String message = getC64ConfigBaseMessage();
    message += configC64Str;
    message += String(END_DELIM);
    return message;
}

void DashioDevice::addChartLineInts(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, int lineData[], int dataLength) {
    addControlBaseMessage(message, CHART_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += lineName;
    message += String(DELIM);
    addLineTypeStr(message, lineType);
    message += String(DELIM);
    message += color;
    message += String(DELIM);
    addYaxisSelectStr(message, yAxisSelect);
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        message += formatInt(lineData[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addChartLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, float lineData[], int dataLength) {
    addControlBaseMessage(message, CHART_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += lineName;
    message += String(DELIM);
    addLineTypeStr(message, lineType);
    message += String(DELIM);
    message += color;
    message += String(DELIM);
    addYaxisSelectStr(message, yAxisSelect);
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        message += formatFloat(lineData[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addTimeGraphLineBaseMessage(String& message, const String& _dashboardID, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect) {
    addControlBaseMessage(message, TIME_GRAPH_ID, controlID);
    message += _dashboardID;
    message += String(DELIM);
    message += lineID;
    message += String(DELIM);
    message += lineName;
    message += String(DELIM);
    addLineTypeStr(message, lineType);
    message += String(DELIM);
    message += color;
    message += String(DELIM);
    addYaxisSelectStr(message, yAxisSelect);
}

String DashioDevice::getTimeGraphLine(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect) {
    String message = "";
    addTimeGraphLineBaseMessage(message, "BRDCST", controlID, lineID, lineName, lineType, color, yAxisSelect);
    message += String(END_DELIM);
    return message;
}

void DashioDevice::addTimeGraphLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, String times[], float lineData[], int dataLength) {
    addTimeGraphLineBaseMessage(message, dashboardID, controlID, lineID, lineName, lineType, color, yAxisSelect);
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        message += times[i];
        message += ",";
        message += formatFloat(lineData[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addTimeGraphLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, time_t times[], float lineData[], int dataLength, bool breakLine) {
    addTimeGraphLineBaseMessage(message, dashboardID, controlID, lineID, lineName, lineType, color, yAxisSelect);
    char timeBuf[21];
    if (breakLine && (dataLength > 0)) {
        message += String(DELIM);
        time_t breakTime = times[0] - 1; // - 1 second for break point time
        strftime(timeBuf, 21, "%Y-%m-%dT%H:%M:%SZ", localtime(&breakTime));
        message += String(timeBuf);
        message += ",";
        message += "B";
    }
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        strftime(timeBuf, 21, "%Y-%m-%dT%H:%M:%SZ", localtime(&times[i]));
        message += String(timeBuf);
        message += ",";
        message += formatFloat(lineData[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addTimeGraphLineFloatsArr(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, time_t times[], float **lineData, int dataLength, int arrSize) {
    addTimeGraphLineBaseMessage(message, dashboardID, controlID, lineID, lineName, lineType, color, yAxisSelect);
    char timeBuf[21];
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        strftime(timeBuf, 21, "%Y-%m-%dT%H:%M:%SZ", localtime(&times[i]));
        message += String(timeBuf);
        message += ",";
        message += "[";
        for (int j = 0; j < arrSize; j++) {
            if (j > 0) {
                message += ",";
            }
            message += formatFloat(lineData[i][j]);
        }
        message += "]";
    }
    message += String(END_DELIM);
}

void DashioDevice::addTimeGraphLineBools(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, String times[], bool lineData[], int dataLength) {
    addTimeGraphLineBaseMessage(message, dashboardID, controlID, lineID, lineName, lineType, color, yLeft);
    for (int i = 0; i < dataLength; i++) {
        message += String(DELIM);
        message += times[i];
        message += ",";
        if (lineData[i]) {
            message += "T";
        } else {
            message += "F";
        }
    }
    message += String(END_DELIM);
}

String DashioDevice::getTimeGraphPoint(const String& controlID, const String& lineID, float value) {
    String message = getControlBaseMessage(TIME_GRAPH_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

String DashioDevice::getTimeGraphPoint(const String& controlID, const String& lineID, String time, float value) {
    String message = getControlBaseMessage(TIME_GRAPH_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += time;
    message += ",";
    message += formatFloat(value);
    message += String(END_DELIM);
    return message;
}

void DashioDevice::addTimeGraphPointArr(String& message, const String& controlID, const String& lineID, float value[], int arrSize) {
    addControlBaseMessage(message, TIME_GRAPH_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += "[";
    for (int i = 0; i < arrSize; i++) {
        if (i > 0) {
            message += ",";
        }
        message += formatFloat(value[i]);
    }
    message += "]";
    message += String(END_DELIM);
}

void DashioDevice::addTimeGraphPointArr(String& message, const String& controlID, const String& lineID, String time, float value[], int arrSize) {
    addControlBaseMessage(message, TIME_GRAPH_ID, controlID);
    message += lineID;
    message += String(DELIM);
    message += time;
    message += ",";
    message += "[";
    for (int i = 0; i < arrSize; i++) {
        if (i != 0) {
            message += ",";
        }
        message += formatFloat(value[i]);
    }
    message += "]";
    message += String(END_DELIM);
}

String DashioDevice::getControlTypeStr(ControlType controltype) {
    switch (controltype) {
        case connect: return CONNECT_ID;
        case who: return WHO_ID;
        case ctrl: return CTRL_ID;
        case status: return STATUS_ID;
        case dashClock: return CLOCK_ID;
        case config: return CONFIG_ID;
        case storeAndForward: return STORE_AND_FORWARD_ID;
              
        case device: return DEVICE_ID;
        case deviceView: return DEVICE_VIEW_ID;
        case label: return LABEL_ID;
        case button: return BUTTON_ID;
        case menu: return MENU_ID;
        case buttonGroup: return BUTTON_GROUP_ID;
        case eventLog: return EVENT_LOG_ID;
        case slider: return SLIDER_ID;
        case knob: return KNOB_ID;
        case dial: return DIAL_ID;
        case direction: return DIRECTION_ID;
        case textBox: return TEXT_BOX_ID;
        case selector: return SELECTOR_ID;
        case chart: return CHART_ID;
        case timeGraph: return TIME_GRAPH_ID;
        case mapper: return MAP_ID;
        case colorPicker: return COLOR_ID;
        case audioVisual: return AV_ID;
              
        case deviceName: return DEVICE_NAME_ID;
        case wifiSetup: return WIFI_SETUP_ID;
        case tcpSetup: return TCP_SETUP_ID;
        case dashioSetup: return DASHIO_SETUP_ID;
        case mqttSetup: return MQTT_SETUP_ID;
        case resetDevice: return RESET_DEVICE_ID;

        case mqttConn: return MQTT_CONNECTION_ID;
        case bleConn: return BLE_CONNECTION_ID;
        case tcpConn: return TCP_CONNECTION_ID;
              
        case alarmNotify: return ALARM_ID;

        case initModule: return INIT_MODULE_ID;

        case pushToken: return "";
        case unknown: return "";
    }
    return "";
}

ControlType DashioDevice::getControlType(String controltypeStr) {
    if (controltypeStr == CONNECT_ID) {
        return connect;
    } else if (controltypeStr == WHO_ID) {
        return who;
    } else if (controltypeStr == CTRL_ID) {
        return ctrl;
    } else if (controltypeStr == STATUS_ID) {
        return status;
    } else if (controltypeStr == CLOCK_ID) {
        return dashClock;
    } else if (controltypeStr == CONFIG_ID) {
        return config;   
    } else if (controltypeStr == DEVICE_ID) {
        return device;
    } else if (controltypeStr == DEVICE_VIEW_ID) {
        return deviceView;
    } else if (controltypeStr == LABEL_ID) {
        return label;
    } else if (controltypeStr == BUTTON_ID) {
        return button;
    } else if (controltypeStr == MENU_ID) {
        return menu;
    } else if (controltypeStr == BUTTON_GROUP_ID) {
        return buttonGroup;
    } else if (controltypeStr == EVENT_LOG_ID) {
        return eventLog;
    } else if (controltypeStr == SLIDER_ID) {
        return slider;
    } else if (controltypeStr == KNOB_ID) {
        return knob;
    } else if (controltypeStr == DIAL_ID) {
        return dial;
    } else if (controltypeStr == DIRECTION_ID) {
        return direction;
    } else if (controltypeStr == TEXT_BOX_ID) {
        return textBox;
    } else if (controltypeStr == SELECTOR_ID) {
        return selector;
     } else if (controltypeStr == CHART_ID) {
       return chart;
    } else if (controltypeStr == TIME_GRAPH_ID) {
        return timeGraph;
    } else if (controltypeStr == MAP_ID) {
        return mapper;
    } else if (controltypeStr == COLOR_ID) {
        return colorPicker;
    } else if (controltypeStr == AV_ID) {
        return audioVisual;
    } else if (controltypeStr == DEVICE_NAME_ID) {
        return deviceName;
    } else if (controltypeStr == WIFI_SETUP_ID) {
        return wifiSetup;
    } else if (controltypeStr == TCP_SETUP_ID) {
        return tcpSetup;
    } else if (controltypeStr == DASHIO_SETUP_ID) {
        return dashioSetup;
    } else if (controltypeStr == MQTT_SETUP_ID) {
        return mqttSetup;
    } else if (controltypeStr == RESET_DEVICE_ID) {
        return resetDevice;
    } else if (controltypeStr == MQTT_CONNECTION_ID) {
        return mqttConn;
    } else if (controltypeStr == BLE_CONNECTION_ID) {
        return bleConn;
    } else if (controltypeStr == TCP_CONNECTION_ID) {
        return tcpConn;
    } else if (controltypeStr == ALARM_ID) {
        return alarmNotify;
    } else if (controltypeStr == INIT_MODULE_ID) {
        return initModule;
    }
    return unknown;
}

String DashioDevice::getMQTTSubscribeTopic(const String& userName) {
    mqttSubscrberTopic = getMQTTTopic(userName, control_topic);
    return mqttSubscrberTopic;
}

String DashioDevice::getMQTTTopic(const String& userName, MQTTTopicType topic) {
    String tip;
    switch (topic) {
        case data_topic:
            tip = DATA_TOPIC_TIP;
            break;
        case control_topic:
            tip = CONTROL_TOPIC_TIP;
            break;
        case alarm_topic:
            tip = ALARM_TOPIC_TIP;
            break;
        case announce_topic:
            tip = ANNOUNCE_TOPIC_TIP;
            break;
        case will_topic:
            tip = WILL_TOPIC_TIP;
            break;
    }
    return userName + "/" + deviceID + "/" + tip;
}

void DashioDevice::addLineTypeStr(String& message, LineType lineType) {
    switch (lineType) {
        case line:
            message += LINE_ID;
            break;
        case bar:
            message += BAR_GRAPH_ID;
            break;
        case segBar:
            message += SEGMENTED_BAR_ID;
            break;
        case peakBar:
            message +=  PEAK_BAR_ID;
            break;
        case bln:
            message +=  BOOL_ID;
            break;
        default:
            message +=  LINE_ID;
            break;
    }
}

void DashioDevice::addYaxisSelectStr(String& message, YAxisSelect yAxisSelect) {
    switch (yAxisSelect) {
        case yLeft:
            message +=  LEFT_ID;
            break;
        default:
            message +=  RIGHT_ID;
            break;
    }
}

void DashioDevice::addIntArray(String& message, int idata[], int dataLength) {
    for (int i = 0; i < dataLength; i++) {
        if (i > 0) {
            message += String(DELIM);
        }
        message += formatInt(idata[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addFloatArray(String& message, float fdata[], int dataLength) {
    for (int i = 0; i < dataLength; i++) {
        if (i > 0) {
            message += String(DELIM);
        }
        message += formatFloat(fdata[i]);
    }
    message += String(END_DELIM);
}

void DashioDevice::addWaypointJSON(String& message, Waypoint waypoint) {
    DashJSON json;
    json.start();
    if (waypoint.time.length() > 0) {
        json.addKeyString(F("time"), waypoint.time);
    }
    if (waypoint.avgeSpeed.length() > 0) {
        json.addKeyString(F("avgeSpeed"), waypoint.avgeSpeed);
    }
    if (waypoint.peakSpeed.length() > 0) {
        json.addKeyString(F("peakSpeed"), waypoint.peakSpeed);
    }
    if (waypoint.course.length() > 0) {
        json.addKeyString(F("course"), waypoint.course);
    }
    if (waypoint.altitude.length() > 0) {
        json.addKeyString(F("altitude"), waypoint.altitude);
    }
    if (waypoint.distance.length() > 0) {
        json.addKeyString(F("distance"), waypoint.distance);
    }
    json.addKeyString(F("latitude"), waypoint.latitude);
    json.addKeyString(F("longitude"), waypoint.longitude, true);
    message += json.jsonStr;
}

void DashioDevice::addEventJSON(String& message, Event event) {
    DashJSON json;
    json.start();
    json.addKeyString(F("time"), event.time);
    json.addKeyString(F("color"), event.color);
    json.addKeyStringArray(F("lines"), event.lines, event.numLines, true);
    message += json.jsonStr;
}
