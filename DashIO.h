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

#ifndef DashIO_h
#define DashIO_h

#include "Arduino.h"
#include <limits.h>

//???#define DASH_SERVER "dash.dashio.io"
extern char DASH_SERVER[];
#define DASH_PORT 8883

#define SMALLEST_FLOAT_VALUE 0.1e-10
#define INVALID_FLOAT_VALUE 0xFFFFFFFF
#define INVALID_INT_VALUE INT_MAX

enum ConnectionType {
    TCP_CONN,
    BLE_CONN,
    MQTT_CONN
};

enum ControlType {
    who,
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
    graph,
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

struct BLEConnCfg {
    String serviceUUID;           // BLE Service UUID
    String readUUID;              // BLE read characteristic UUID
    String writeUUID;             // BLE write characteristic UUID
    
    BLEConnCfg(String _serviceUUID, String _readUUID, String _writeUUID) {
        serviceUUID = _serviceUUID;
        readUUID = _readUUID;
        writeUUID = _writeUUID;
    }
};

struct TCPConnCfg {
    String ipAddress;             // current IP address of the connection
    int port = 5650;                     // TCP port
    
    TCPConnCfg(String _ipAddress, int _port) {
        ipAddress = _ipAddress;
        port = _port;
    }
};

struct MQTTConnCfg {
    String userName;              // Username for the MQTT host
    String hostURL;               // URL of the MQTT host
    
    MQTTConnCfg(String _userName, String _hostURL) {
        userName = _userName;
        hostURL = _hostURL;
    }
};

struct DeviceCfg {
    int    numDeviceViews = 0;
    String deviceSetup;
    
    DeviceCfg(int _numDeviceViews, String _deviceSetup = "") {
        numDeviceViews = _numDeviceViews;
        deviceSetup = _deviceSetup;
    }
};

struct AlarmCfg {
    String controlID;
    String description;
    String soundName;

    AlarmCfg(String _controlID, String _description, String _soundName = "Default") {
        controlID = _controlID;
        description = _description;
        soundName = _soundName;
    }
};

struct DeviceViewCfg {
    String controlID;             // Unique identifier of the Device View.
    String title;                 // Device View Name
    String iconName;              // Name of the icon from the icons in IoT Dashboard. Displayed on the menu.
    String color = "black";       // Device View background color. Color name from colors in IoT Dashboard e.g. "blue" or index
    bool   shareColumn = true;    // When true, allows device views to share a column if their height is small enough.
    int    numColumns = 1;        // Number of columns wide the deviceView is (1 to 3).
     
    // Control Default Values
    int    ctrlMaxFontSize = 30;  // The maximum size of the text font. The font size will never be larger than this value.
    bool   ctrlBorderOn = true;   // Default Hide or show border on all controls
    String ctrlBorderColor = "white"; // Default color of the border and title box text. Color name from colors in IoT Dashboard e.g. "blue" or index
    String ctrlColor = "white";   // Default color of misc parts of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    String ctrlBkgndColor = "blue"; // Default color of the background of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    int    ctrlBkgndTransparency = 0; // Default transparency of the background of all controls (0 to 100).

    // Control Title Box Default Values
    int    ctrlTitleFontSize = 18; // Font size for the title (8 to 30)
    String ctrlTitleBoxColor = "blue"; // Default color of the title box of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    int    ctrlTitleBoxTransparency = 0; // Default transparency of the title box of all controls (0 to 100).
    
    DeviceViewCfg(String _controlID, String _title, String _iconName, String _color) {
        controlID = _controlID;
        title = _title;
        iconName = _iconName;
        color = _color;
    }
};

struct CommonControl {
    String controlID;             // Identifier of the control.
    String parentID;              // Identifier of the parent control
    String title;                 // Text to be displayed in the title box of the control
    TitlePosition titlePosition = titleTop; // Default position of title box. "titleOff", "titleTop" or "titleBottom"
    Rect   graphicsRect;          // Position and size of the control
    
    CommonControl(String _controlID, String _parentID, String _title, Rect _graphicsRect) {
        controlID = _controlID;
        parentID = _parentID;
        title = _title;
        graphicsRect =_graphicsRect;
    }
};

struct LabelCfg : CommonControl {
    LabelStyle style = group;     // Style of the label. May be "basic", "border" or "group"
    String color = "white";       // Color of the text
    
    LabelCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
             : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct ButtonCfg : CommonControl {
    bool   buttonEnabled = true;  // Hide or show the slider
    String iconName;              // Name of the icon from the icons in IoT Dashboard. Displayed on the button.
    String text;                  // Text displayed on the button
    String offColor = "dark gray"; // Color of text or icon when the button is in the OFF state. Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    String onColor = "white";     // Color of text or icon when the button is in the ON state. Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"

    ButtonCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
              : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct MenuCfg : CommonControl {
    String iconName = "menu";     // Name of the icon from the icons in IoT Dashboard. Displayed on the button.
    String text;                  // Text displayed on the menu control

    MenuCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
            : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct ButtonGroupCfg : CommonControl {
    String iconName = "group";    // Name of the icon from the icons in IoT Dashboard. Displayed on the button.
    String text;                  // Text displayed on the button group control
    bool   gridView = true;       // If true, show as a grid of buttons, otherwise show as a vertical menu.

    ButtonGroupCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                   : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct EventLogCfg : CommonControl {
    EventLogCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct KnobCfg : CommonControl {
    float  min = 0;               // Minimum value of the dial behind the knob
    float  max = 100;             // Maximum value of the dial behind the knob
    float  redValue = 70;         // When the dial value is above this number it will be colored red
    bool   showMinMax = true;     // Show the maximum and minimum values on the dial
    KnobPresentationStyle style = knobNormal; // Presentation style of the dial. May be "knobNormal" or "knobPan"
    String knobColor = "red";     // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    bool   sendOnlyOnRelease = true; // Send message only when the the knob is released
    bool   dialFollowsKnob = true; // The dial behind the knob shows the same value as the knob
    String dialColor = "yellow";  // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"

    KnobCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
            : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct DialCfg : CommonControl {
    float  min = 0;               // Minimum value of the dial
    float  max = 100;             // Maximum value of the dial
    float  redValue = 70;         // When the dial value is above this number it will be colored red
    String dialFillColor = "green"; // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    String pointerColor = "yellow"; // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    DialNumberPosition numberPosition = numberCenter; // Position of the display of the value. May be "numberOff", "numberLeft", "numberRight" or numberCenter
    bool   showMinMax = true;     // Show the maximum and minimum values on the dial.
    DialPresentationStyle style = dialBar; // Presentation style of the dial. May be "pie", "dialPieInverted" or "bar"
    String units;                 // To be displayed after the text
    int precision = 3;            // Numeric precision - number of charactes excluding the decimal point from 1 to 6. Any other value = off

    DialCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
            : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct DirectionCfg : CommonControl {
    String pointerColor = "yellow"; // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    DirectionPresentationStyle style = dirNSEW; // Presentation style of the dial. May be "nsew", "deg" or "degps"
    int    calAngle = 0;          // Correction offset in degrees for the direction pointer
    String units;                 // To be displayed below the text
    int precision = 3;            // Numeric precision - number of charactes excluding the decimal point from 1 to 6. Any other value = off

    DirectionCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                 : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct TextBoxCfg : CommonControl {
    TextFormat format = noFmt;          // numFmt, dateTimeFmt, dateTimeLongFmt, intvlFmt, noFmt

    TextAlign textAlign = textCenter; // textCenter, textLeft or textRight
    String units;                 // To be displayed after the text
    int precision = 0;            // Numeric precision - number of charactes excluding the decimal point from 1 to 6. Any other value = off
    KeyboardType kbdType = allKbd; // "none", "allKbd" for all characters, "numKbd" for numeric, or "hexKbd" for hexidecimal, intKbd, dateKbd, timeKbd, dateTimeKbd, intvlKbd,
    bool   closeKbdOnSend = true; // Hide the keyboard once the message has been sent.

    TextBoxCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
               : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct SelectorCfg : CommonControl {
    SelectorCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct SliderCfg : CommonControl {
    float  min = 0;               // Minimum value of the slider and bar
    float  max = 100;             // Maximum value of the slider and bar
    float  redValue = 70;         // When the bar value is above this number it will be colored red
    bool   showMinMax = true;     // Show the maximum and minimum values on the bar and slider
    bool   sliderEnabled = true;  // Hide or show the slider
    String knobColor = "white";   // Color of the knob of the slider
    bool   sendOnlyOnRelease = true; // Send message only when the the knob is released
    bool   barFollowsSlider = true; // The bar behind the slider shows the same value as the knob
    String barColor = "green";    // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    BarStyle barStyle = segmentedBar; // "solidBar" or "segmentedBar" for a bar divided into segments

    SliderCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
              : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct GraphCfg : CommonControl {
    String xAxisLabel;            // Title of the horizontal axis
    float  xAxisMin = 0;          // Value of the horizontal axis left hand side
    float  xAxisMax = 100;        // Value of the horizontal axis right hand side
    int    xAxisNumBars = 6;      // Number of horizontal grid lines, including the left and right axis.
    XAxisLabelsStyle xAxisLabelsStyle = labelsOnLines; // "labelsOnLines" for on the grid lines, or "labelsBetweenLines" for between the grid lines
    String yAxisLabel;            // Title of the vertical axis
    float  yAxisMin = 0;          // Value of the vertical axis bottom
    float  yAxisMax = 100;        // Value of the vertical axis top
    int    yAxisNumBars = 6;      // Number of vertical grid lines, including the top and bottom axes.

    GraphCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
             : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct TimeGraphCfg : CommonControl {
    String yAxisLabel;            // Title of the horizontal axis
    float  yAxisMin = 0;          // Value of the vertical axis bottom
    float  yAxisMax = 100;        // Value of the vertical axis top
    int    yAxisNumBars = 6;      // Number of vertical grid lines, including the top and bottom axes.

    TimeGraphCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                 : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct MapCfg : CommonControl {
    MapCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
           : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct ColorCfg : CommonControl {
    ColorPickerStyle pickerStyle = wheel;
    bool sendOnlyOnRelease = true; // Send message only when the the knob is released

    ColorCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
             : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct AudioVisualCfg : CommonControl {
    AudioVisualCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
             : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
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
    String getReceivedMessageForPrint(const String& controlStr);

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

    DashioDevice(const String& _deviceType);
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
    String getTextBoxMessage(const String& controlID, const String& text);
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
    String getEventLogMessage(const String& controlID, const String& timeStr, const String& color, String text[], int numTextRows);
    String getEventLogMessage(const String& controlID, Event events[], int numEvents);
    String getColorMessage(const String& controlID, const String& color);
    String getAudioVisualMessage(const String& controlID, const String& url = "");

    String getGraphLineInts(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, int lineData[], int dataLength);
    String getGraphLineFloats(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, float lineData[], int dataLength);
    String getTimeGraphLineFloats(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, String times[], float lineData[], int dataLength, bool breakLine = false);
    String getTimeGraphLineBools(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color, String times[], bool lineData[], int dataLength);
    String getTimeGraphPoint(const String& controlID, const String& graphLineID, float value);
    String getTimeGraphPoint(const String& controlID, const String& graphLineID, String time, float value);
    String getTimeGraphLine(const String& controlID, const String& graphLineID, const String& lineName, LineType lineType, const String& color);

//  Config messages
    String getBasicConfigData(ControlType controlType, const String& controlID, const String& controlTitle);
    String getBasicConfigMessage(ControlType controlType, const String& controlID, const String& controlTitle);
    String getBasicConfigMessage(const String& configData);
    String getFullConfigMessage(ControlType controlType, const String& configData);

    String getConfigMessage(DeviceCfg deviceConfigData);
    String getConfigMessage(DeviceViewCfg deviceViewData);

    String getConfigMessage(BLEConnCfg MessageData);
    String getConfigMessage(TCPConnCfg MessageData);
    String getConfigMessage(MQTTConnCfg MessageData);

    String getConfigMessage(AlarmCfg alarmData);

    String getConfigMessage(LabelCfg labelData);
    String getConfigMessage(ButtonCfg buttonData);
    String getConfigMessage(MenuCfg menuData);
    String getConfigMessage(ButtonGroupCfg groupData);
    String getConfigMessage(EventLogCfg eventLogData);
    String getConfigMessage(KnobCfg knobData);
    String getConfigMessage(DialCfg dialData);
    String getConfigMessage(DirectionCfg directionData);
    String getConfigMessage(TextBoxCfg textBoxData);
    String getConfigMessage(SelectorCfg selectorData);
    String getConfigMessage(SliderCfg sliderData);
    String getConfigMessage(GraphCfg graphData);
    String getConfigMessage(TimeGraphCfg timeGraphData);
    String getConfigMessage(MapCfg mapData);
    String getConfigMessage(ColorCfg colorData);
    String getConfigMessage(AudioVisualCfg avData);

    String getOnlineMessage();
    String getOfflineMessage();

    String getControlTypeStr(ControlType controltype);

    String getMQTTSubscribeTopic(const String& userName);
    String getMQTTTopic(const String& userName, MQTTTopicType topic);
    
private:
    String getControlBaseMessage(const String& messageType, const String& controlID);
    String getLineTypeStr(LineType lineType);
    String getIntArray(const String& controlType, const String& ID, int idata[], int dataLength);
    String getFloatArray(const String& controlType, const String& ID, float fdata[], int dataLength);

    String getWaypointJSON(Waypoint waypoint);
    String getEventJSON(Event event);

    String getTitlePositionStr(TitlePosition tbp);
    String getLabelStyle(LabelStyle labelStyle);
    String getKnobPresentationStyle(KnobPresentationStyle presentationStyle);
    String getDialNumberPosition(DialNumberPosition numberPosition);
    String getDialPresentationStyle(DialPresentationStyle presentationStyle);
    String getDirectionPresentationStyle(DirectionPresentationStyle presentationStyle);
    String getTextFormatStr(TextFormat format);
    String getKeyboardTypeStr(KeyboardType kbd);
    String getTextAlignStr(TextAlign align);
    String getBarStyleStr(BarStyle barStyle);
    String getXAxisLabelsStyleStr(XAxisLabelsStyle xls);
    String getColorStyleStr(ColorPickerStyle pickerStyle);
};

#endif

