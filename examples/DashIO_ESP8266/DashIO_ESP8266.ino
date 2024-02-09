#include "DashioESP.h"
#include "DashioProvisionESP.h"

#include <Arduino-timer.h>
#include <NTPClient.h>
#include <TimeLib.h>

#define DEVICE_TYPE "ESP8266_Dash"
#define DEVICE_NAME "Dash8266_Master"

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

const char configC64Str[] PROGMEM =
"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";


DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);
DashProvision dashProvision(&dashDevice);

// dash comms connections
DashWiFi wifi;
DashTCP  tcp_con(&dashDevice, true);
DashMQTT mqtt_con(&dashDevice, true, true);

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

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, 0);

static String getUTCtime() {
    time_t time = timeClient.getEpochTime();
    if (time > 1000) {
        char buffer[22];
        sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(time), month(time), day(time), hour(time), minute(time), second(time)); // Z = UTC time
        String dateTime(buffer);
        return dateTime;
    } else {
        return "";
    }
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
        tcp_con.sendMessage(message);
    } else {
        mqtt_con.sendMessage(message);
    }
}

void sendMessageAll(const String& message) {
    tcp_con.sendMessage(message);
    mqtt_con.sendMessage(message);
}

// Process incoming control mesages
void processStatus(ConnectionType connectionType) {
  String message((char *)0);
  message.reserve(1024);

  // Control initial condition messages (recommended)
  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  message = dashDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += dashDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += dashDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += dashDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);

  message += dashDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += dashDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += dashDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += dashDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += dashDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);

  int data1[] = {150, 270, 390, 410, 400};
  dashDevice.addChartLineInts(message, GRAPH_ID, "L1", "Line One", line, "3", yLeft, data1, sizeof(data1)/sizeof(int));
  int data2[] = {160, 280, 400, 410, 420};
  dashDevice.addChartLineInts(message, GRAPH_ID, "L2", "Line Two", line, "4", yLeft, data2, sizeof(data2)/sizeof(int));
  int data3[] = {170, 290, 410, 400, 390};
  dashDevice.addChartLineInts(message, GRAPH_ID, "L3", "Line Three", line, "5", yLeft, data3, sizeof(data3)/sizeof(int));
  int data4[] = {180, 270, 390, 410, 430};
  dashDevice.addChartLineInts(message, GRAPH_ID, "L4", "Line Four", line, "6", yLeft, data4, sizeof(data4)/sizeof(int));
  int data5[] = {200, 250, 260, 265, 240};
  dashDevice.addChartLineInts(message, GRAPH_ID, "L5", "Line Five", line, "8", yLeft, data5, sizeof(data5)/sizeof(int));
  sendMessage(connectionType, message);
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == MENU_BUTTON_ID) {
        menuButtonValue = !menuButtonValue;
        String message = dashDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
        sendMessage(messageData->connectionType, message);
    } else if (messageData->idStr == BGROUP_B01_ID) {
        buttonOneValue = !buttonOneValue;
        String message = dashDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
        sendMessage(messageData->connectionType, message);

        message = dashDevice.getAlarmMessage("AL03", "An Alarm", "This is a test alarm");
        mqtt_con.sendAlarmMessage(message);
    } else if (messageData->idStr == BUTTON02_ID) {
        String textArr[] = {"one", "two"};
        String message = "";
        dashDevice.addEventLogMessage(message, LOG_ID, getUTCtime(), "red", textArr, 2);
        sendMessageAll(message);
    }
}


void setupWiFiMQTT();

void onWiFiConnectCallback(void) {
    timeClient.begin();
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
      processStatus(messageData->connectionType);
      break;
    case button:
      processButton(messageData);
      break;
    case textBox:
      if (messageData->idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = messageData->payloadStr.toFloat();
        String message = dashDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        sendMessage(messageData->connectionType, message);
      }
      break;
    case slider:
      if (messageData->idStr == MENU_SLIDER_ID) {
        menuSliderValue = messageData->payloadStr.toFloat();
        String message = dashDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        sendMessage(messageData->connectionType, message);
      }
      break;
    case selector:
      if (messageData->idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = messageData->payloadStr.toInt();
        String message = dashDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        sendMessage(messageData->connectionType, message);
      }
      break;
    case knob:
      if (messageData->idStr == KNOB_ID) {
        String message = dashDevice.getDialMessage(DIAL_ID, (int)messageData->payloadStr.toInt());
        sendMessage(messageData->connectionType, message);
      }
      break;
    case timeGraph:
      break;
    default:
        dashProvision.processMessage(messageData);
        break;
    }
}

void checkOutgoingMessages() {
    if (messageToSend.length() > 0) {
        tcp_con.sendMessage(messageToSend);
        mqtt_con.sendMessage(messageToSend);
        messageToSend = "";
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (commsChanged) {
        delay(2); // Make sure last message is sent before restarting comms
        wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);
        mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
    } else {
        mqtt_con.sendWhoAnnounce(); // Update announce topic with new name
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    messageToSend.reserve(200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashDevice.setup(wifi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashDevice.deviceID);

    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);

    wifi.setOnConnectCallback(&onWiFiConnectCallback);

    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
    wifi.attachConnection(&mqtt_con);
    
    wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);

    timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    timeClient.update();
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
            messageToSend = dashDevice.getMapWaypointMessage(MAP_ID, "TZ1", "-43.559860", "172.655620");
        }
    }
}
