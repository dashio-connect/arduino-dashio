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
    device,
    page,
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
    deviceName,
    wifiSetup,
    dashSetup,
    popupMessage,
    unknown
};

enum LineType {
  line,
  bar,
  segBar,
  peakBar
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
    numberRight
};

enum DialPresentationStyle {
    dialUpright,
    dialInverted
};

enum KeyboardType {
    hexChars,
    allChars,
    numChars,
    none
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
    int port;                     // TCP port
    
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
    int    numPages;
    String deviceSetup;
    
    DeviceCfg(int _numPages, String _deviceSetup = "") {
        numPages = _numPages;
        deviceSetup = _deviceSetup;
    }
};

struct PageCfg {
    String controlID;             // Unique identifier of the page.
    String title;                 // Page Name
    String iconName;              // Name of the icon from the icons in IoT Dashboard. Displayed on the page menu.
    String pageColor;             // Color of the page. Color name from colors in IoT Dashboard e.g. "blue" or index
     
    // Control Default Values
    int    ctrlMaxFontSize = 30;  // The maximum size of the text font. The font size will never be larger than this value.
    bool   ctrlBorderOn = true;   // Default Hide or shor border on all controls
    String ctrlBorderColor = "white"; // Default color of the border and title boxtext. Color name from colors in IoT Dashboard e.g. "blue" or index
    String ctrlColor = "white";   // Default color of misc parts of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    String ctrlBkgndColor;        // Default color of the background of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    int    ctrlBkgndTransparency = 0; // Default transparency of the background of all controls (0 to 100).

    // Control Title Box Default Values
    int    ctrlTitleFontSize = 18; // Font size for the title (8 to 30)
    String ctrlTitleBoxColor;     // Default color of the title box of all controls. Color name from colors in IoT Dashboard e.g. "blue" or index
    int    ctrlTitleBoxTransparency = 0; // Default transparency of the title box of all controls (0 to 100).
    
    PageCfg(String _controlID, String _title, String _iconName, String _pageColor) {
        controlID = _controlID;
        title = _title;
        iconName = _iconName;
        pageColor = _pageColor;
    }
};

struct CommonControl {
    String controlID;             // Identifier of the control.
    String parentID;              // Identifier of the parent page or control
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
    DialNumberPosition numberPosition = numberRight; // Position of the display of the value. May be "numberOff", "numberLeft" or "numberRight"
    bool   showMinMax = true;     // Show the maximum and minimum values on the dial.
    DialPresentationStyle style = dialUpright; // Presentation style of the dial. May be "dialUpright" or "dialInverted"
    String units;                 // To be displayed after the text
    int precision = 3;            // Numeric precision - number of charactes excluding the decimal point from 1 to 6. Any other value = off

    DialCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
            : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct DirectionCfg : CommonControl {
    String pointerColor = "yellow"; // Color name from colors in IoT Dashboard e.g. "blue" or index to the color e.g. "4"
    int    calAngle = 0;          // Correction offset in degrees for the direction pointer

    DirectionCfg(String _controlID, String _parentID, String _title, Rect _graphicsRect = Rect())
                 : CommonControl(_controlID, _parentID, _title, _graphicsRect) {}
};

struct TextBoxCfg : CommonControl {
    TextAlign textAlign = textCenter; // textCenter, textLeft or textRight
    String units;                 // To be displayed after the text
    int precision = 0;            // Numeric precision - number of charactes excluding the decimal point from 1 to 6. Any other value = off
    KeyboardType kbdType = numChars; // "none", "allChars" for all characters, "nnumCharsum" for numeric, or "hexChars" for hexidecimal
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


class DashConnection {
  public:
    ConnectionType connectionType;
    bool messageReceived = false;
    String deviceID = "";
    ControlType control = unknown;
    String idStr = "";
    String payloadStr = "";

    DashConnection(ConnectionType connType);
    void processMessage(String message);
    bool processChar(char chr);

  private:
    int segmentCount = -1;
    String readStr;
};

class DashDevice {
  public:
    String mqttSubscrberTopic;
    String deviceID = "";
    
    void setDeviceID(uint8_t m_address[6]);
    void setDeviceID(String deviceIdentifier);

    String getWhoMessage(String deviceName, String deviceType);
    String getConnectMessage();
    String getPopupMessage(String header, String body = "", String caption = "");
    String getDeviceNameMessage(String deviceName);
    String getAlarmMessage(String alarmID, String title, String description);
    String getAlarmMessage(Notification alarm);
    String getButtonMessage(String controlID, bool on, String iconName = "", String text = "");
    String getTextBoxMessage(String controlID, String text);
    String getSelectorMessage(String controlID, int index);
    String getSelectorMessage(String controlID, int index, String* selectionItems, int rows);
    String getSliderMessage(String controlID, int value);
    String getSingleBarMessage(String controlID, int value);
    String getKnobMessage(String controlID, int value);
    String getKnobDialMessage(String controlID, int value);
    String getDirectionMessage(String controlID, int value);
    String getDialMessage(String controlID, String text);
    String getMapMessage(String controlID, String latitude, String longitude, String message);
    String getEventLogMessage(String controlID, String timeStr, String color, String text[], int dataLength);
    String getDoubleBarMessage(String controlID, int value1, int value2);
    String getBasicConfigData(ControlType controlType, String controlID, String controlTitle);
    String getBasicConfigMessage(ControlType controlType, String controlID, String controlTitle);
    String getBasicConfigMessage(String configData);
    String getFullConfigMessage(ControlType controlType, String configData);
    
    String getGraphLineInts(String controlID, String graphLineID, String lineName, LineType lineType, String color, int lineData[], int dataLength);
    String getGraphLineFloats(String controlID, String graphLineID, String lineName, LineType lineType, String color, float lineData[], int dataLength);
    String getTimeGraphLineFloats(String controlID, String graphLineID, String lineName, LineType lineType, String color, String times[], float lineData[], int dataLength);
    String getTimeGraphLineBools(String controlID, String graphLineID, String lineName, LineType lineType, String color, String times[], bool lineData[], int dataLength);
    
    //  Config messages
    String getConfigMessage(DeviceCfg deviceConfigData);
    String getConfigMessage(PageCfg pageData);

    String getConfigMessage(BLEConnCfg connectionData);
    String getConfigMessage(TCPConnCfg connectionData);
    String getConfigMessage(MQTTConnCfg connectionData);

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

        
        
    String getOnlineMessage();
    String getOfflineMessage();
    
    String getControlTypeStr(ControlType controltype);

    String getMQTTSubscribeTopic(String userName);
    String getMQTTTopic(String userName, MQTTTopicType topic);
    
  private:
    String getLineTypeStr(LineType lineType);
    String getIntArray(String controlType, String ID, int idata[], int dataLength);
    String getFloatArray(String controlType, String ID, float fdata[], int dataLength);

    String getTitlePositionStr(TitlePosition tbp);
    String getLabelStyle(LabelStyle labelStyle);
    String getKnobPresentationStyle(KnobPresentationStyle presentationStyle);
    String getDialNumberPosition(DialNumberPosition numberPosition);
    String getDialPresentationStyle(DialPresentationStyle presentationStyle);
    String getKeyboardTypeStr(KeyboardType kbd);
    String getTextAlignStr(TextAlign align);
    String getBarStyleStr(BarStyle barStyle);
    String getXAxisLabelsStyleStr(XAxisLabelsStyle xls);
};

#endif

