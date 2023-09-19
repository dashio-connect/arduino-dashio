/*
 DashIO.h - Library for the DashIO comms protocol.
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

#ifndef Dashio_h
#define Dashio_h

#include "Arduino.h"
#include <limits.h>

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

extern char DASH_SERVER[];
#define DASH_PORT 8883

#define SMALLEST_FLOAT_VALUE 0.1e-10
#define INVALID_FLOAT_VALUE 0xFFFFFFFF
#define INVALID_INT_VALUE INT_MAX

enum ConnectionType {
    TCP_CONN,
    BLE_CONN,
    MQTT_CONN,
    SERIAL_CONN
};

enum ControlType {
    who,
    ctrl,
    connect,
    status,
    config,
    pushToken,
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

enum TitlePosition {
    titleOff,
    titleTop,
    titleBottom
};

enum LabelStyle {
    basic,
    border,
    group
};

enum KnobPresentationStyle {
    knobNormal,
    knobPan
};

enum DialNumberPosition {
    numberOff,
    numberLeft,
    numberRight,
    numberCenter,
};

enum DialPresentationStyle {
    dialPie,
    dialPieInverted,
    dialBar
};

enum DirectionPresentationStyle {
    dirNSEW,
    dirDeg,
    dirDegPS
};

enum TextFormat {
    numFmt,
    dateTimeFmt,
    dateTimeLongFmt,
    intvlFmt,
    noFmt
};

enum KeyboardType {
    hexKbd,
    allKbd,
    numKbd,
    intKbd,
    dateKbd,
    timeKbd,
    dateTimeKbd,
    intvlKbd,
    noKbd
};

enum TextAlign {
    textLeft,
    textCenter,
    textRight
};

enum BarStyle {
    segmentedBar,
    solidBar
};

enum XAxisLabelsStyle {
    labelsOnLines,
    labelsBetweenLines
};

enum ColorPickerStyle {
    wheel,
    spectrum
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

    ControlType control = unknown;
    String idStr = ((char *)0);
    String payloadStr = ((char *)0);
    String payloadStr2 = ((char *)0);
/*
    String payloadStr3 = ((char *)0);
    String payloadStr4 = ((char *)0);
*/

    MessageData(ConnectionType connType);
    void processMessage(const String& message);
    bool processChar(char chr);
    String getMessageGeneric(const String& controlStr);
    String getReceivedMessageForPrint(const String& controlStr);
    String getTransmittedMessageForPrint(const String& controlStr);

private:
    int segmentCount = -1;
    String readStr;
};

class DashioDevice {
public:
    String mqttSubscrberTopic;
    String deviceID = ((char *)0);
    String type = ((char *)0);
    String name = ((char *)0);
    String dashboardID = "BRDCST";
    const char *configC64Str = NULL;
    unsigned int cfgRevision = 0;
    
    DashioDevice(const String& _deviceType, const char *_configC64Str = NULL, unsigned int cfgRevision = 0);
    void setup(const String& deviceIdentifier);
    void setup(const String& deviceIdentifier, const String& _deviceName);
    void setup(uint8_t m_address[6]);
    void setup(uint8_t m_address[6], const String& _deviceName);

    String getWhoMessage();
    String getConnectMessage();

    String getDeviceNameMessage();
    String getWifiUpdateAckMessage();
    String getTCPUpdateAckMessage();
    String getDashioUpdateAckMessage();
    String getMQTTUpdateAckMessage();

    String getAlarmMessage(const String& alarmID, const String& title, const String& description);
    String getAlarmMessage(Notification alarm);
    String getButtonMessage(const String& controlID, bool on, const String& iconName = "", const String& text = "");
    String getTextBoxMessage(const String& controlID, const String& text, const String& color = "");
    String getSelectorMessage(const String& controlID, int index);
    String getSelectorMessage(const String& controlID, int index, String* selectionItems, int numItems);

    String getSliderMessage(const String& controlID, int value);
    String getSliderMessage(const String& controlID, float value);
    String getSingleBarMessage(const String& controlID, int value);
    String getSingleBarMessage(const String& controlID, float value);
    String getDoubleBarMessage(const String& controlID, int value1, int value2);
    String getDoubleBarMessage(const String& controlID, float value1, float value2);

    String getKnobMessage(const String& controlID, int value);
    String getKnobMessage(const String& controlID, float value);
    String getKnobDialMessage(const String& controlID, int value);
    String getKnobDialMessage(const String& controlID, float value);
    String getDialMessage(const String& controlID, int value);
    String getDialMessage(const String& controlID, float value);
    String getDirectionMessage(const String& controlID, int direction, float speed = -1);
    String getDirectionMessage(const String& controlID, float direction, float speed = -1);
    String getMapWaypointMessage(const String& controlID, const String& trackID, const String& latitude, const String& longitude);
    String getMapTrackMessage(const String& controlID, const String& trackID, const String& text, const String& colour, Waypoint waypoints[] = {}, int numWaypoints = 0);
    String getEventLogMessage(const String& controlID, const String& color, String text[], int numTextRows);
    String getEventLogMessage(const String& controlID, const String& timeStr, const String& color, String text[], int numTextRows);
    String getEventLogMessage(const String& controlID, Event events[], int numEvents);
    String getColorMessage(const String& controlID, const String& color);
    String getAudioVisualMessage(const String& controlID, const String& url = "");

    String getChartLineInts(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, int lineData[], int dataLength);
    String getChartLineFloats(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, float lineData[], int dataLength);
    String getTimeGraphLineFloats(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect, String times[], float lineData[], int dataLength, bool breakLine = false);
    String getTimeGraphLineBools(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, String times[], bool lineData[], int dataLength);
    String getTimeGraphPoint(const String& controlID, const String& lineID, float value);
    String getTimeGraphPoint(const String& controlID, const String& lineID, String time, float value);
    String getTimeGraphLine(const String& controlID, const String& lineID, const String& lineName, LineType lineType, const String& color, YAxisSelect yAxisSelect);

//  Config messages
    String getC64ConfigBaseMessage();
    String getC64ConfigMessage();

    String getOnlineMessage();
    String getOfflineMessage();
    
    String getDataStoreEnableMessage(DashStore dashStore);

    String getControlTypeStr(ControlType controltype);

    String getMQTTSubscribeTopic(const String& userName);
    String getMQTTTopic(const String& userName, MQTTTopicType topic);
    
private:
    String getControlBaseMessage(const String& messageType, const String& controlID);
    String getLineTypeStr(LineType lineType);
    String getYaxisSelectStr(YAxisSelect yAxisSelect);
    String getIntArray(int idata[], int dataLength);
    String getFloatArray(float fdata[], int dataLength);

    String getWaypointJSON(Waypoint waypoint);
    String getEventJSON(Event event);
};

#endif

