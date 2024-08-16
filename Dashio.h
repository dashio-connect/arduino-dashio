/*
 Dashio.h - Library for the DashIO comms protocol.
 Created by C. Tuffnell, Dashio Connect Limited
 
 The Dashio library provides the common functionality for other Dash libraries.
 There are two important classes:
 1) MessageData - processing incoming messages and storing the message data
    for each connection.
 2) DashioDevice - contains informtion about the device and functions for
    creating messages from the device information.
 
 For more information, visit: https://dashio.io/documents/
 
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

// Notes on String usage: https://www.forward.com.au/pfod/ArduinoProgramming/ArduinoStrings/index.html

#ifndef Dashio_h
#define Dashio_h

#include "Arduino.h"
#include <limits.h>
#include <time.h>

class DashioDevice;
typedef DashioDevice DashDevice;

class DashioWiFi;
typedef DashioWiFi DashWiFi;

class DashioBLE;
typedef DashioBLE DashBLE;

class DashioTCP;
typedef DashioTCP DashTCP;

class DashioMQTT;
typedef DashioMQTT DashMQTT;

class DashioProvision;
typedef DashioProvision DashProvision;

class DashioBluefruit_BLE;
typedef DashioBluefruit_BLE DashBluefruit_BLE;

class DashioBluno;
typedef DashioBluno DashBluno;

class DashioSoftAP;
typedef DashioSoftAP DashSoftAP;

class DashioLTE;
typedef DashioLTE DashLTE;

extern char DASH_SERVER[];
#define DASH_PORT 8883
#define DEFAULT_TCP_PORT 5650

#define SMALLEST_FLOAT_VALUE 0.1e-10
#define INVALID_FLOAT_VALUE 0xFFFFFFFF
#define INVALID_INT_VALUE INT_MAX

#define DEFAULT_DEVICE_NAME "DashIO Device"

const char END_DELIM = '\n';
const char DELIM = '\t';
const char NOT_AVAILABLE[] = "NA";
const char DTAG[] = "DASHIO";

enum ConnectionType {
    TCP_CONN,
    BLE_CONN,
    MQTT_CONN,
    SERIAL_CONN,
    ALL_CONN
};

enum ControlType {
    who,
    ctrl,
    connect,
    dashClock,
    status,
    config,
    pushToken,
    storeAndForward,

    mqttConn,
    bleConn,
    tcpConn,
    alarmNotify,

    device,
    deviceView,
    label,
    button,
    menu,
    buttonGroup,
    eventLog,
    slider,
    knob,
    dial,
    direction,
    textBox,
    selector,
    chart,
    timeGraph,
    mapper,
    colorPicker,
    audioVisual,

    deviceName,
    wifiSetup,
    tcpSetup,
    dashioSetup,
    mqttSetup,
    initModule,
    resetDevice,

    unknown
};

enum LineType {
    line,
    bar,
    segBar,
    peakBar,
    bln
};

enum YAxisSelect {
    yLeft,
    yRight
};

enum ButtonMultiState {
    off,
    on
};

enum MQTTTopicType {
    data_topic,
    control_topic,
    alarm_topic,
    announce_topic,
    will_topic
};

enum StatusCode {
    noError,
    wifiConnected,
    wifiDisconnected,
    mqttConnected,
    mqttDisconnected
};

struct Rect {
    float  xPositionRatio;        // Position of the left side of the control as a ratio of the screen width (0 to 1)
    float  yPositionRatio;        // Position of the top side of the control as a ratio of the screen height (0 to 1)
    float  widthRatio;            // Width of the control as a ratio of the screen width (0 to 1)
    float  heightRatio;           // Height of the control as a ratio of the screen height (0 to 1)
};

struct Notification {
    String identifier;            // Identifier of the notification.
    String title;                 // Displayed in the notification title.
    String description;           // Displayed in the notification body.
};

struct Waypoint {
    String time;                  // yyyy-MM-dd’T’HH:mm:ssZ (refer to ISO 8601)
    String latitude;              // Latitude in decimal degrees
    String longitude;             // Longitude in decimal degrees
    String avgeSpeed;             // Average speed since the last message in meters/second
    String peakSpeed;             // Maximum speed since the last message in meters/second
    String course;                // Course direction in decimal degrees. A negative value indicates an unknown heading
    String altitude;              // Altitude in meters
    String distance;              // Accumulated distance since the last message in meters
};

struct Event {
    String time;                  // yyyy-MM-dd’T’HH:mm:ssZ (refer to ISO 8601)
    String color;                 // Color name from colors in IoT Dashboard e.g. "blue" or index
    String *lines;                // Lines of text
    int numLines;                 // Number of lines of text
};

struct DashStore {
    ControlType controlType;
    String      controlID;
};

class MessageData {
public:
    ConnectionType connectionType;
    bool messageReceived = false;
    String deviceID = ((char *)0);
    
    bool checkPrefix = false;
    ControlType control = unknown;
    String idStr = ((char *)0);
    String payloadStr = ((char *)0);
    String payloadStr2 = ((char *)0);
    uint16_t connectionHandle = 0;
    /*
     String payloadStr3 = ((char *)0);
     String payloadStr4 = ((char *)0);
     */
    
    MessageData(ConnectionType connType, int _bufferLength = 0);
    void processMessage(const String& message, uint16_t _connectionHandle = 0);
    bool processChar(char chr);
    String getMessageGeneric(const String& controlStr, bool connectionPrefix = false);
    String getReceivedMessageForPrint(const String& controlStr);
    String getTransmittedMessageForPrint(const String& controlStr);
    String getConnectionTypeStr();
    
    void checkBuffer();
    
private:
    char* buffer = nullptr;
    int bufferWritePtr = 0;
    int bufferReadPtr = 0;
    int bufferLength = 0;
    int segmentCount = -1;
    String readStr;
    
    void loadBuffer(const String& message, uint16_t _connectionHandle);
};

class DashioDevice {
public:
    String mqttSubscrberTopic;
    String deviceID = ((char *)0);
    String type = ((char *)0);
    String name = ((char *)0);
    String dashboardID = "BRDCST";
    const char *configC64Str = nullptr;
    unsigned int cfgRevision = 0;
    
    DashioDevice(const String& _deviceType, const char *_configC64Str = nullptr, unsigned int cfgRevision = 0);
    void setup(const String& deviceIdentifier);
    void setup(const String& deviceIdentifier, const String& _deviceName);
    void setup(uint8_t m_address[6]);
    void setup(uint8_t m_address[6], const String& _deviceName);
    
    void appendDelimitedStr(String *str, const String& addStr);

    void setStatusCallback(void (*_statusCallback)(StatusCode statusCode));
    void (*statusCallback)(StatusCode statusCode) = nullptr; // Only used for ESP32
    void onStatusCallback(StatusCode statusCode);

    String getWhoMessage();
    String getConnectMessage();
    String getClockMessage();

    String getDeviceNameMessage();
    String getWifiUpdateAckMessage();
    String getTCPUpdateAckMessage();
    String getDashioUpdateAckMessage();
    String getMQTTUpdateAckMessage();
    String getResetDeviceMessage();

    String getAlarmMessage(const String& alarmID, const String& title, const String& description);
    String getAlarmMessage(Notification alarm);

    String getButtonMessage(const String& controlID);
    String getButtonMessage(const String& controlID, bool on, const String& iconName = "", const String& text = "");

    String getTextBoxMessage(const String& controlID, const String& text, const String& color = "");
    String getTextBoxCaptionMessage(const String& controlID, const String& text, const String& color = "");

    String getSelectorMessage(const String& controlID);
    String getSelectorMessage(const String& controlID, int index);
    String getSelectorMessage(const String& controlID, int index, String selectionItems[], int numItems);
    String getSelectorMessage(const String& controlID, int index, const String& selectionStr);

    String getSliderMessage(const String& controlID, int value);
    String getSliderMessage(const String& controlID, float value);
    String getSliderMessage(const String& controlID);
    String getSingleBarMessage(const String& controlID, int value);
    String getSingleBarMessage(const String& controlID, float value);
    String getSingleBarMessage(const String& controlID);
    String getDoubleBarMessage(const String& controlID, int value1, int value2);
    String getDoubleBarMessage(const String& controlID, float value1, float value2);
    String getDoubleBarMessage(const String& controlID);

    String getKnobMessage(const String& controlID, int value);
    String getKnobMessage(const String& controlID, float value);
    String getKnobMessage(const String& controlID);
    String getKnobDialMessage(const String& controlID, int value);
    String getKnobDialMessage(const String& controlID, float value);
    String getKnobDialMessage(const String& controlID);

    String getDialMessage(const String& controlID, int value);
    String getDialMessage(const String& controlID, float value);
    String getDialMessage(const String& controlID);

    String getDirectionMessage(const String& controlID, int direction, float speed = -1);
    String getDirectionMessage(const String& controlID, float direction, float speed = -1);
    String getDirectionMessage(const String& controlID);

    String getMapWaypointMessage(const String& controlID, const String& trackID, const String& latitude, const String& longitude);
    String getMapWaypointMessage(const String& controlID, const String& trackID, float latitude, float longitude);
    String getMapTrackMessage(const String& controlID, const String& trackID, const String& text, const String& colour, Waypoint waypoints[] = {}, int numWaypoints = 0);

    void addEventLogMessage(String& message, const String& controlID, const String& color, String text[], int numTextRows);
    void addEventLogMessage(String& message, const String& controlID, const String& timeStr, const String& color, String text[], int numTextRows);
    void addEventLogMessage(String& message, const String& controlID, Event events[], int numEvents);

    String getColorMessage(const String& controlID, const String& color);

    String getAudioVisualMessage(const String& controlID, const String& url = "");

    void addChartLineInts(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, int lineData[], int dataLength);
    void addChartLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, float lineData[], int dataLength);

    String getTimeGraphLine(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect);
    void addTimeGraphLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, String times[], float lineData[], int dataLength);
    void addTimeGraphLineFloats(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, time_t times[], float lineData[], int dataLength, bool breakLine = false);
    void addTimeGraphLineFloatsArr(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, time_t times[], float **lineData, int dataLength, int arrSize);
    void addTimeGraphLineBools(String& message, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, String times[], bool lineData[], int dataLength);

    String getTimeGraphPoint(const String& controlID, const String& lineID, float value);
    String getTimeGraphPoint(const String& controlID, const String& lineID, String time, float value);
    void addTimeGraphPointArr(String& message, const String& controlID, const String& lineID, float value[], int arrSize);
    void addTimeGraphPointArr(String& message, const String& controlID, const String& lineID, String time, float value[], int arrSize);

//  Config messages
    String getC64ConfigBaseMessage();
    String getC64ConfigMessage(); //??? Obsolete - remove in due course

    String getOnlineMessage();
    String getOfflineMessage();
    
    String getDataStoreEnableMessage(DashStore dashStore);

    String getControlTypeStr(ControlType controltype);
    ControlType getControlType(String controltypeStr);

    String getMQTTSubscribeTopic(const String& userName);
    String getMQTTTopic(const String& userName, MQTTTopicType topic);
    
private:
    String getControlBaseMessage(const String& messageType, const String& controlID);
    void addControlBaseMessage(String& message, const String& messageType, const String& controlID);
    void addLineTypeStr(String& message, LineType lineType);
    void addYaxisSelectStr(String& message, YAxisSelect yAxisSelect);
    void addTimeGraphLineBaseMessage(String& message, const String& _dashboardID, const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect);
    void addIntArray(String& message, int idata[], int dataLength);
    void addFloatArray(String& message, float fdata[], int dataLength);

    void addWaypointJSON(String& message, Waypoint waypoint);
    void addEventJSON(String& message, Event event);
};

#endif

