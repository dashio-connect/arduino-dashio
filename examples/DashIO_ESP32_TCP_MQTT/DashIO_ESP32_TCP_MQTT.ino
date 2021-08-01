#define DEVICE_TYPE "ESP32_Baby"
#define DEVICE_NAME "DashIO32_Baby"
#define PRNT

#include "DashIO_ESP32_TCP_MQTT.h"
#include <WiFiUdp.h>
#include <TimeLib.h>

// WiFi
#define WIFI_SSID      "KotukuBlue" //???"yourWiFiSSID"
#define WIFI_PASSWORD  "Judecat12" //???"yourWiFiPassword"

// MQTT
#define MQTT_USER      "craig" //???"yourMQTTuserName"
#define MQTT_PASSWORD  "ThingyThing" //???"yourMQTTpassword"

// Create controls
int menuSelectorIndex = 0;
float menuTextBoxValue = 12.34;
bool menuButtonValue = on;
bool buttonOneValue = on;
bool buttonTwoValue = off;
bool buttonThreeValue = on;
bool buttonFourValue = off;
bool buttonFiveValue = on;
float menuSliderValue = 14;

// Create Control IDs
// Device Views
const char *DV01_ID = "DV01";
const char *DV02_ID = "DV02";

// Device View 1 Controls
const char *LOG_ID = "EL01";
const char *BUTTON02_ID = "UB02";
const char *MAP_ID = "MP01";

const char *MENU_ID = "M01";
const char *MENU_SELECTOR_ID = "SLR01";
const char *MENU_BUTTON_ID = "UB01";
const char *MENU_TEXTBOX_ID = "CTB01";
const char *MENU_SLIDER_ID = "SLD01";

const char *BGROUP_ID = "BG01";
const char *BGROUP_B01_ID = "GB01";
const char *BGROUP_B02_ID = "GB02";
const char *BGROUP_B03_ID = "GB03";
const char *BGROUP_B04_ID = "GB04";
const char *BGROUP_B05_ID = "GB05";

// Device View 2 Controls
const char *GRAPH_ID = "IDG";
const char *LABEL_ID = "LBL01";
const char *KNOB_ID = "KB01";
const char *DIAL_ID = "DL01";

const char* ntpServer = "pool.ntp.org";

String getLocalTime() {
  struct tm dt; // dateTime
  if(!getLocalTime(&dt)){
    Serial.println(F("Failed to obtain time"));
    return "";
  }
  char buffer[22];
  sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", 1900 + dt.tm_year, dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec); // Z = UTC time
  String dateTime(buffer);
  return dateTime;
}

static String timeToString(long time) {
  char buffer[22];
  sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(time), month(time), day(time), hour(time), minute(time), second(time)); // Z = UTC time
  String dateTime(buffer);
  return dateTime;
}

// Process incoming control mesages
void processStatus(ConnectionType connectionType) {
  // Control initial condition messages (recommended)
  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
  sendMessage(connectionType, message);

  message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += myDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += myDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += myDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += myDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);
  sendMessage(connectionType, message);

  int data1[] = {150, 270, 390, 410, 400};
  sendMessage(connectionType, myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int)));
  int data2[] = {160, 280, 400, 410, 420};
  sendMessage(connectionType, myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int)));
  int data3[] = {170, 290, 410, 400, 390};
  sendMessage(connectionType, myDevice.getGraphLineInts(GRAPH_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int)));
  int data4[] = {180, 270, 390, 410, 430};
  sendMessage(connectionType, myDevice.getGraphLineInts(GRAPH_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int)));
  int data5[] = {190, 280, 380, 370, 360};
  int data6[] = {200, 250, 260, 265, 240};
  sendMessage(connectionType, myDevice.getGraphLineInts(GRAPH_ID, "L6", "Line Six", line, "8", data6, sizeof(data6)/sizeof(int)));
}

void processConfig(ConnectionType connectionType) {
    sendMessage(connectionType, myDevice.getConfigMessage(DeviceCfg(2, "name, wifi, dashio")));  // Two Device Views

    EventLogCfg anEventLog(LOG_ID, DV01_ID, "Log", {0.05, 0.545, 0.9, 0.212});
    sendMessage(connectionType, myDevice.getConfigMessage(anEventLog));

    MapCfg aMap(MAP_ID, DV01_ID, "My Map", {0.0, 0.0, 1.0, 0.515});
    sendMessage(connectionType, myDevice.getConfigMessage(aMap));

    ButtonCfg aButton(BUTTON02_ID, DV01_ID, "Event", {0.5, 0.788, 0.2, 0.121});
    aButton.iconName = "bell";
    aButton.offColor = "blue";
    aButton.onColor = "black";
    sendMessage(connectionType, myDevice.getConfigMessage(aButton));

    MenuCfg aMenu(MENU_ID, DV01_ID, "Settings", {0.05, 0.788, 0.4, 0.121});
    aMenu.iconName = "pad";
    aMenu.text = "Setup";
    sendMessage(connectionType, myDevice.getConfigMessage(aMenu));

    SelectorCfg menuSelector(MENU_SELECTOR_ID, MENU_ID, "Selector");
    sendMessage(connectionType, myDevice.getConfigMessage(menuSelector));

    ButtonCfg menuButton(MENU_BUTTON_ID,  MENU_ID, "On/Off");
    menuButton.offColor = "red";
    menuButton.onColor = "purple";
    sendMessage(connectionType, myDevice.getConfigMessage(menuButton));

    TextBoxCfg menuTextBox(MENU_TEXTBOX_ID, MENU_ID, "Counter");
    menuTextBox.units = "°C";
    sendMessage(connectionType, myDevice.getConfigMessage(menuTextBox));

    SliderCfg menuSlider(MENU_SLIDER_ID, MENU_ID, "UV");
    menuSlider.knobColor = "green";
    menuSlider.barColor = "yellow";
    sendMessage(connectionType, myDevice.getConfigMessage(menuSlider));

    ButtonGroupCfg aGroup(BGROUP_ID, DV01_ID, "Actions", {0.75, 0.788, 0.2, 0.121});
    aGroup.iconName = "pad";
    aGroup.text = "BG";
    sendMessage(connectionType, myDevice.getConfigMessage(aGroup));

    ButtonCfg groupButton1(BGROUP_B01_ID, BGROUP_ID, "Up");
    groupButton1.iconName = "up";
    groupButton1.offColor = "red";
    groupButton1.onColor = "purple";
    sendMessage(connectionType, myDevice.getConfigMessage(groupButton1));

    ButtonCfg groupButton2(BGROUP_B02_ID, BGROUP_ID, "Down");
    groupButton2.iconName = "down";
    groupButton2.offColor = "Green";
    groupButton2.onColor = "#9d9f2c";
    sendMessage(connectionType, myDevice.getConfigMessage(groupButton2));

    ButtonCfg groupButton3(BGROUP_B03_ID, BGROUP_ID, "Left");
    groupButton3.iconName = "left";
    groupButton3.offColor = "#123456";
    groupButton3.onColor = "#228855";
    sendMessage(connectionType, myDevice.getConfigMessage(groupButton3));

    ButtonCfg groupButton4(BGROUP_B04_ID, BGROUP_ID, "Right");
    groupButton4.iconName = "right";
    groupButton4.offColor = "red";
    groupButton4.onColor = "#F4A37C";
    sendMessage(connectionType, myDevice.getConfigMessage(groupButton4));

    ButtonCfg groupButton5(BGROUP_B05_ID, BGROUP_ID, "Stop");
    groupButton5.iconName = "stop";
    groupButton5.offColor = "Black";
    groupButton5.onColor = "#bc41d7";
    sendMessage(connectionType, myDevice.getConfigMessage(groupButton5));
    
    GraphCfg aGraph(GRAPH_ID, DV02_ID, "Level", {0.0, 0.0, 1.0, 0.485});
    aGraph.xAxisLabel = "Temperature";
    aGraph.xAxisMin = 0;
    aGraph.xAxisMax = 1000;
    aGraph.xAxisNumBars = 6;
    aGraph.yAxisLabel = "Mixing Rate";
    aGraph.yAxisMin = 100;
    aGraph.yAxisMax = 600;
    aGraph.yAxisNumBars = 6;
    sendMessage(connectionType, myDevice.getConfigMessage(aGraph));
    
    LabelCfg aLabel(LABEL_ID, DV02_ID, "Label", {0.0, 0.515, 1.0, 0.394});
    aLabel.color = 22;
    sendMessage(connectionType, myDevice.getConfigMessage(aLabel));
    
    KnobCfg aKnob(KNOB_ID, DV02_ID, "Knob", {0.05, 0.576, 0.4, 0.303});
    aKnob.knobColor = "#FF00F0";
    aKnob.dialColor = "yellow";
    sendMessage(connectionType, myDevice.getConfigMessage(aKnob));
    
    DialCfg aDial(DIAL_ID, DV02_ID, "Dial", {0.55, 0.576, 0.4, 0.303});
    aDial.dialFillColor = "green";
    aDial.pointerColor = "yellow";
    aDial.style = dialPieInverted;
    aDial.units = "F";
    aDial.precision = 2;
    sendMessage(connectionType, myDevice.getConfigMessage(aDial));
    
    TCPConnCfg tcpCnctnConfig(ip2CharArray(WiFi.localIP()), TCP_PORT);
    sendMessage(connectionType, myDevice.getConfigMessage(tcpCnctnConfig));
    
    MQTTConnCfg mqttCnctnConfig(userSetup.dashUserName, MQTT_SERVER);
    sendMessage(connectionType, myDevice.getConfigMessage(mqttCnctnConfig));
    
    AlarmCfg alarmCfg("AID1", "A useful description", "PlopPlipPlip");
    sendMessage(connectionType, myDevice.getConfigMessage(alarmCfg));
    
    // Device Views
    DeviceViewCfg deviceViewOne(DV01_ID, "First Device View", "down", "dark gray");
    deviceViewOne.ctrlBkgndColor = "blue";
    deviceViewOne.ctrlTitleBoxColor = 5;
    sendMessage(connectionType, myDevice.getConfigMessage(deviceViewOne));
    
    DeviceViewCfg deviceViewTwo(DV02_ID, "Second Device View", "up", "0");
    deviceViewTwo.ctrlBkgndColor = "blue";
    deviceViewTwo.ctrlTitleBoxColor = 5;
    sendMessage(connectionType, myDevice.getConfigMessage(deviceViewTwo));
}

void processButton(DashConnection *connection) {
    if (connection->idStr == MENU_BUTTON_ID) {
        menuButtonValue = !menuButtonValue;
        String message = myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
        sendMessage(connection->connectionType, message);
    } else if (connection->idStr == BGROUP_B01_ID) {
        buttonOneValue = !buttonOneValue;
        String message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
        sendMessage(connection->connectionType, message);

        message = myDevice.getAlarmMessage("AL03", "An Alarm", "This is a test alarm");
        sendAlarmMessage(message);
    } else if (connection->idStr == BUTTON02_ID) {
        String localTimeStr = getLocalTime();
        String textArr[] = {"one", "two"};
        String message = myDevice.getEventLogMessage(LOG_ID, localTimeStr, "red", textArr, 2);
        sendMessage(connection->connectionType, message);
    }
}

void onIncomingMessageCallback(DashConnection *connection) {
  switch (connection->control) {
    case status:
      processStatus(connection->connectionType);
      break;
    case config:
      processConfig(connection->connectionType);
      break;
    case button:
      processButton(connection);
      break;
    case textBox:
      if (connection->idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = connection->payloadStr.toFloat();
        String message = myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        sendMessage(connection->connectionType, message);
      }
      break;
    case slider:
      if (connection->idStr == MENU_SLIDER_ID) {
        menuSliderValue = connection->payloadStr.toFloat();
        String message = myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        sendMessage(connection->connectionType, message);
      }
      break;
    case selector:
      if (connection->idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = connection->payloadStr.toInt();
        String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        sendMessage(connection->connectionType, message);
      }
      break;
    case knob:
      if (connection->idStr == KNOB_ID) {
        String message = myDevice.getDialMessage(DIAL_ID, connection->payloadStr);
        sendMessage(connection->connectionType, message);
      }
      break;
    case timeGraph:
      break;
  }
}

void onWiFiConnectCallback() {
    configTime(0, 0, ntpServer); // utcOffset = 0 and daylightSavingsOffset = 0, so time is UTC
    time_t now;
    long currentTime;
    time(&currentTime);
    
    Serial.print(F("NTP Time: "));
    Serial.println(timeToString(currentTime));
}
  
void setup() {
    Serial.begin(115200);

    ProvisioningObject defaultUserSetup = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD, 'N'};
    userSetup = defaultUserSetup;
    loadUserSetup();

    setupDevice(&onIncomingMessageCallback);
    wifiConnectCallback = &onWiFiConnectCallback;
}

void loop() {
    messageToSend = myDevice.getMapMessage(MAP_ID, "-43.559880", "172.655620", "12345");
    delay(1000);
}
