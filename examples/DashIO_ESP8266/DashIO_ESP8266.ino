#include "DashioESP.h"
#include "DashioProvisionESP.h"

#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Arduino-timer.h>

//#define NO_TCP
//#define NO_MQTT

#define DEVICE_TYPE "ESP8266_DashIO"
#define DEVICE_NAME "DashIO8266_Baby"

#define TCP_PORT 5000
#define MQTT_BUFFER_SIZE 2048

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

DashioDevice dashioDevice(DEVICE_TYPE);
DashioProvision dashioProvision(&dashioDevice);

// DashIO comms connections
    DashioWiFi wifi;
#ifndef NO_TCP
    DashioTCP  tcp_con(&dashioDevice, TCP_PORT, true);
#endif
#ifndef NO_MQTT
    DashioMQTT mqtt_con(&dashioDevice, MQTT_BUFFER_SIZE, true, true);
#endif

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
auto timer = timer_create_default();
bool oneSecond = false; // Set by hardware timer every second.
int count = 0;
String messageToSend = ((char *)0);

/*???
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
*/

static String timeToString(long time) {
  char buffer[22];
  sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(time), month(time), day(time), hour(time), minute(time), second(time)); // Z = UTC time
  String dateTime(buffer);
  return dateTime;
}

char *ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

// Timer Interrupt
static bool onTimerCallback(void *argument) {
  oneSecond = true;
  return true; // to repeat the timer action - false to stop
}

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == TCP_CONN) {
#ifndef NO_TCP
        tcp_con.sendMessage(message);
#endif
    } else {
#ifndef NO_MQTT
        mqtt_con.sendMessage(message);
#endif
    }
}

// Process incoming control mesages
void processStatus(ConnectionType connectionType) {
  String message((char *)0);
  message.reserve(1024);

  // Control initial condition messages (recommended)
  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  message = dashioDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += dashioDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += dashioDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += dashioDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);

  message += dashioDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += dashioDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += dashioDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += dashioDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += dashioDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);

  int data1[] = {150, 270, 390, 410, 400};
  message += dashioDevice.getGraphLineInts(GRAPH_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int));
  int data2[] = {160, 280, 400, 410, 420};
  message += dashioDevice.getGraphLineInts(GRAPH_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int));
  int data3[] = {170, 290, 410, 400, 390};
  message += dashioDevice.getGraphLineInts(GRAPH_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int));
  int data4[] = {180, 270, 390, 410, 430};
  message += dashioDevice.getGraphLineInts(GRAPH_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int));
  int data5[] = {200, 250, 260, 265, 240};
  message += dashioDevice.getGraphLineInts(GRAPH_ID, "L5", "Line Five", line, "8", data5, sizeof(data5)/sizeof(int));
  sendMessage(connectionType, message);
}

void processFirstConfigBlock(ConnectionType connectionType) {
    String message((char *)0);
    message.reserve(2048);

    message += dashioDevice.getConfigMessage(DeviceCfg(2, "name, wifi, dashio")); // Two Device Views
    EventLogCfg anEventLog(LOG_ID, DV01_ID, "Log", {0.05, 0.545, 0.9, 0.212});
    
    message += dashioDevice.getConfigMessage(anEventLog);

    MapCfg aMap(MAP_ID, DV01_ID, "My Map", {0.0, 0.0, 1.0, 0.515});
    message += dashioDevice.getConfigMessage(aMap);

    ButtonCfg aButton(BUTTON02_ID, DV01_ID, "Event", {0.5, 0.788, 0.2, 0.121});
    aButton.iconName = "bell";
    aButton.offColor = "blue";
    aButton.onColor = "black";
    message += dashioDevice.getConfigMessage(aButton);

    MenuCfg aMenu(MENU_ID, DV01_ID, "Settings", {0.05, 0.788, 0.4, 0.121});
    aMenu.iconName = "pad";
    aMenu.text = "Setup";
    message += dashioDevice.getConfigMessage(aMenu);

    SelectorCfg menuSelector(MENU_SELECTOR_ID, MENU_ID, "Selector");
    message += dashioDevice.getConfigMessage(menuSelector);

    ButtonCfg menuButton(MENU_BUTTON_ID,  MENU_ID, "On/Off");
    menuButton.offColor = "red";
    menuButton.onColor = "purple";
    message += dashioDevice.getConfigMessage(menuButton);
    
    sendMessage(connectionType, message);

    TextBoxCfg menuTextBox(MENU_TEXTBOX_ID, MENU_ID, "Counter");
    menuTextBox.units = "°C";
    message = dashioDevice.getConfigMessage(menuTextBox);

    SliderCfg menuSlider(MENU_SLIDER_ID, MENU_ID, "UV");
    menuSlider.knobColor = "green";
    menuSlider.barColor = "yellow";
    message += dashioDevice.getConfigMessage(menuSlider);

    ButtonGroupCfg aGroup(BGROUP_ID, DV01_ID, "Actions", {0.75, 0.788, 0.2, 0.121});
    aGroup.iconName = "pad";
    aGroup.text = "BG";
    message += dashioDevice.getConfigMessage(aGroup);

    ButtonCfg groupButton1(BGROUP_B01_ID, BGROUP_ID, "Up");
    groupButton1.iconName = "up";
    groupButton1.offColor = "red";
    groupButton1.onColor = "purple";
    message += dashioDevice.getConfigMessage(groupButton1);

    ButtonCfg groupButton2(BGROUP_B02_ID, BGROUP_ID, "Down");
    groupButton2.iconName = "down";
    groupButton2.offColor = "Green";
    groupButton2.onColor = "#9d9f2c";
    message += dashioDevice.getConfigMessage(groupButton2);

    sendMessage(connectionType, message);
}

void processSecondConfigBlock(ConnectionType connectionType) {
    String message((char *)0);
    message.reserve(2048);
    
    ButtonCfg groupButton3(BGROUP_B03_ID, BGROUP_ID, "Left");
    groupButton3.iconName = "left";
    groupButton3.offColor = "#123456";
    groupButton3.onColor = "#228855";
    message = dashioDevice.getConfigMessage(groupButton3);

    ButtonCfg groupButton4(BGROUP_B04_ID, BGROUP_ID, "Right");
    groupButton4.iconName = "right";
    groupButton4.offColor = "red";
    groupButton4.onColor = "#F4A37C";
    message += dashioDevice.getConfigMessage(groupButton4);

    ButtonCfg groupButton5(BGROUP_B05_ID, BGROUP_ID, "Stop");
    groupButton5.iconName = "stop";
    groupButton5.offColor = "Black";
    groupButton5.onColor = "#bc41d7";
    message += dashioDevice.getConfigMessage(groupButton5);
    
    GraphCfg aGraph(GRAPH_ID, DV02_ID, "Level", {0.0, 0.0, 1.0, 0.485});
    aGraph.xAxisLabel = "Temperature";
    aGraph.xAxisMin = 0;
    aGraph.xAxisMax = 1000;
    aGraph.xAxisNumBars = 6;
    aGraph.yAxisLabel = "Mixing Rate";
    aGraph.yAxisMin = 100;
    aGraph.yAxisMax = 600;
    aGraph.yAxisNumBars = 6;
    message += dashioDevice.getConfigMessage(aGraph);
    
    LabelCfg aLabel(LABEL_ID, DV02_ID, "Label", {0.0, 0.515, 1.0, 0.394});
    aLabel.color = 22;
    message += dashioDevice.getConfigMessage(aLabel);

    sendMessage(connectionType, message);
    
    KnobCfg aKnob(KNOB_ID, DV02_ID, "Knob", {0.05, 0.576, 0.4, 0.303});
    aKnob.knobColor = "#FF00F0";
    aKnob.dialColor = "yellow";
    message = dashioDevice.getConfigMessage(aKnob);
    
    DialCfg aDial(DIAL_ID, DV02_ID, "Dial", {0.55, 0.576, 0.4, 0.303});
    aDial.dialFillColor = "green";
    aDial.pointerColor = "yellow";
    aDial.style = dialPieInverted;
    aDial.units = "F";
    aDial.precision = 2;
    message += dashioDevice.getConfigMessage(aDial);
    
    TCPConnCfg tcpCnctnConfig(ip2CharArray(WiFi.localIP()), TCP_PORT);
    message += dashioDevice.getConfigMessage(tcpCnctnConfig);
    
    MQTTConnCfg mqttCnctnConfig(dashioProvision.dashUserName, DASH_SERVER);
    message += dashioDevice.getConfigMessage(mqttCnctnConfig);
    
    AlarmCfg alarmCfg("AID1", "A useful description", "PlopPlipPlip");
    message += dashioDevice.getConfigMessage(alarmCfg);
    
    sendMessage(connectionType, message);

    // Device Views
    DeviceViewCfg deviceViewOne(DV01_ID, "First Device View", "down", "dark gray");
    deviceViewOne.ctrlBkgndColor = "blue";
    deviceViewOne.ctrlTitleBoxColor = 5;
    message = dashioDevice.getConfigMessage(deviceViewOne);
    
    DeviceViewCfg deviceViewTwo(DV02_ID, "Second Device View", "up", "0");
    deviceViewTwo.ctrlBkgndColor = "blue";
    deviceViewTwo.ctrlTitleBoxColor = 5;
    message += dashioDevice.getConfigMessage(deviceViewTwo);

    sendMessage(connectionType, message);
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == MENU_BUTTON_ID) {
        menuButtonValue = !menuButtonValue;
        String message = dashioDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
        sendMessage(messageData->connectionType, message);
    } else if (messageData->idStr == BGROUP_B01_ID) {
        buttonOneValue = !buttonOneValue;
        String message = dashioDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
        sendMessage(messageData->connectionType, message);

        message = dashioDevice.getAlarmMessage("AL03", "An Alarm", "This is a test alarm");
#ifndef NO_MQTT
        mqtt_con.sendAlarmMessage(message);
#endif
    } else if (messageData->idStr == BUTTON02_ID) {
/*???
        String localTimeStr = getLocalTime();
        String textArr[] = {"one", "two"};
        String message = dashioDevice.getEventLogMessage(LOG_ID, localTimeStr, "red", textArr, 2);
        sendMessage(messageData->connectionType, message);
*/
    }
}

void onWiFiConnectCallback(void) {
    configTime(0, 0, ntpServer); // utcOffset = 0 and daylightSavingsOffset = 0, so time is UTC
    time_t now;
    long currentTime;
    time(&currentTime);
    
    Serial.print(F("NTP Time: "));
    Serial.println(timeToString(currentTime));
}

void setupWiFiMQTT();

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
      processStatus(messageData->connectionType);
      break;
    case config:
      processFirstConfigBlock(messageData->connectionType);
      processSecondConfigBlock(messageData->connectionType);
      break;
    case button:
      processButton(messageData);
      break;
    case textBox:
      if (messageData->idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = messageData->payloadStr.toFloat();
        String message = dashioDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        sendMessage(messageData->connectionType, message);
      }
      break;
    case slider:
      if (messageData->idStr == MENU_SLIDER_ID) {
        menuSliderValue = messageData->payloadStr.toFloat();
        String message = dashioDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        sendMessage(messageData->connectionType, message);
      }
      break;
    case selector:
      if (messageData->idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = messageData->payloadStr.toInt();
        String message = dashioDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        sendMessage(messageData->connectionType, message);
      }
      break;
    case knob:
      if (messageData->idStr == KNOB_ID) {
        String message = dashioDevice.getDialMessage(DIAL_ID, (int)messageData->payloadStr.toInt());
        sendMessage(messageData->connectionType, message);
      }
      break;
    case timeGraph:
      break;
    default:
        dashioProvision.processMessage(messageData);
        break;
    }
}

void checkOutgoingMessages() {
    if (messageToSend.length() > 0) {
#ifndef NO_TCP
        tcp_con.sendMessage(messageToSend);
#endif
#ifndef NO_MQTT
        mqtt_con.sendMessage(messageToSend);
#endif
        messageToSend = "";
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (commsChanged) {
        delay(2); // Make sure last message is sent before restarting comms
        wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);
#ifndef NO_MQTT
        mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
#endif
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    messageToSend.reserve(200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashioDevice.setup(wifi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashioDevice.deviceID);

#ifndef NO_TCP
    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);
#endif

#ifndef NO_MQTT
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
    wifi.attachConnection(&mqtt_con);
#endif
    
    wifi.setOnConnectCallback(&onWiFiConnectCallback);
    wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);

    timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    timer.tick();

    wifi.run();
    
    checkOutgoingMessages();

    if (oneSecond) { // Tasks to occur every second
        oneSecond = false;
        count++;
        if (count > 64000) {
            count = 0;
        }
        if (count % 5 == 0) {
            messageToSend = dashioDevice.getMapWaypointMessage(MAP_ID, "TZ1", "-43.559880", "172.655620");
        }
    }
}
