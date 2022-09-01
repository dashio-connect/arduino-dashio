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

const char configC64Str[] PROGMEM =
"zVZZb+o4FP4ryPOKUEKhpbyRsqi6bKIto9GoDyY5BauJnes4BW7V/z7HTgIhi7iVRqORKlSfnO37zmJ/kgB4HJH+35/kuBQRU0zw"
"FcVf0rdad71ek+yZp3YnUadJFFM+ZLqkT54XS9IkzBV8TgNAwZJ6JFXD0xMoxfg2QlFIJXD1OETpcG3ZKEEjJYVvRDMjUXBQiVUc"
"4vFQSsrqNskO2HanTiK7bX+9NolPN+DXYena3Ussdh2SSB1N4pPV4mWZAzLV7kso2gUUU2eaIvOFxPPgZ0yrcJRA3Nx3NAiN3xEH"
"0DjwGNCwBtFvosmynx0bMxpeL8LSiK6ni3zqdDexUoJPpIjr8qzqova3u2jgar3rTeRM8l3kTPD/rWTemsGe9JWMoaql7mpbKoGX"
"IBNvbw9pUR0/hkLgF8e0QmIw4nTjg5dFFPxsSN33ShDl+Gfwow9UJnWc1TCeUkAquC8z0L2g3gHfJxr+VtJwl9Z1cGDRjGFY28Ju"
"OOijmYjoKZ2XxbxAyeNQ0384GaJZ4oUeSP9WeymP0uGkgGGs3PDBhxm+c1yNH4IQJFWxhFp2Crx2et0KxtLEMr8zdsCN1cCv2q/5"
"NI8Dh0qk4jbNIS8oDGKJXk3lOxebhMlsv8wXq9lgWuDsh2O6IQLuLbh/XPAV+EAjyFopSKmp4k6Ct6a6M/t3OeZ+YOAyDzfWTe3+"
"24k91ssUIQlasU3vbhNIWWP/MR5b1thCc49Rfyx8X+wjEzr1ocWZ8l+gP5c6s1O37gPTPZrF4KdSD4JzcJOUP8lOROplNUWvHo12"
"Lf3DRAttmySOQKYdTb6SDJIKhBJcFhkH7fO+Xz6OHufrQj2GyTb/Pu1DjFZDu2GI+Sc6JhKAf2O8DfmhYFyBLFHK42ADMuflYTR/"
"Hq0q6/ob9HeR/pgzhcSRMcmXQl9wsUyvKeWGl2UJhcTt0zUzzMKB50mItA/7vt2yb3st/LOte10W6lMZpG48+GAu6GWdFMpV0sd8"
"x1iQJ/YLab2xNA4sAeKOA54BQdCJIDIDeFHA9IpGT877lnvFFa7lQnogFydvZ1mm/OeOqUy7JCus9vO7B9PwGkMDqWEuoFwaz5Ly"
"yPSSezQLSH951qZ4+WchFqi0hcYKPHKpUG99psruXez0lxDJ/rcYtf8TRodUvjdwOI45VsdMRup/Q+pQ7Lm5KJPGxRF4Y1vd/kjf"
"MN/L7S8NIYBJdqG+6h3v47yIbIQinyE/2SAwCemLx5xBvwCmYlv7uu10i+N8f+1FiO7ItefUaFr9HKx6hbfttqaCxh4TaxbFybZ9"
"/foH";

DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);
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
