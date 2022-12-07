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
"xVjdjuo2EH4V5HOLKAlk2eWO8Fe0BDghcFqd9iIkBqwNMXWcBbrad+oz9Mk6dhIISVjgSGerIKSMx+P5Ps+Mx3lDVvc3CzW/vyGH"
"+pxRb9BBTdS29KqCymhrM+xzKTKkIPQJD+Dt33/a8MYJ97BQp6HPMQPJGpPVmps2JxQ1q7HGhAYEBD5oWuOJmIf3vOWRlZC0uyOr"
"a4JwSdnG5iAZjUddeD8k007Wtgw7JJCW4O1l4VqHrVi/NRzChB1x+fqkvM/Pdzwa4OeFO/an2HdRk7MQv/9ZRp35/JvkIOAHCUhv"
"TQcCoMOZZ9j7HlAzJX/DSA2srBhx29QLNz4woaplFKyBpkgS2SwjP9wcVZTyGbeduWRSmNYpczEDRcpg4NuacJyMvKx8NxnQvTCR"
"X9S1mO0HcrucQ4Q11uzY7KXUZ/iQ2q8eYQEvdfArcXBpTvAutmSJcZ3uk1XGYHSFSyZ2Mwq51QQnJt0B2lotpXoiTnksIwI0jOyN"
"8KBDdz56L38a4+onMq57tvOSYnuKwRn3f6V7tkUiznXLGsk4p8tlsmi8WoquWZT7i5Bz6nd9e+FhN2GZ+sm8Sci2Hi6qEYU1QMDz"
"//hlvFyiS1WhIOFFoYDxm5I7BXdEfSyj60Oc/R/Gqfc/BAps/3yMYkvPEUKSYz+PUb2O8cuT+7RUnbtAygz++TCPhSIF9Iui1ura"
"Qx5q7Qaoqvr4qGl3QR3iJf8EqHKZW6K2fgPMXr1Va7TvgmkK8SfgjNbJAE2KZgaqdgPUhVNX3MZdUKecfkaOymVyQKOj5bzg3pCk"
"CUMplHErcY6yoqjKCWn3FZRvh1p5VOBRLyKuqAWYK9oZah17njxunkdj/aytGo1NozXMoH+OynAAHdnY9w5j38QetgOcMLCx93Ce"
"VatZ5IIyht25LQhtNlK7++zTRZ6WWrV2iYZgTXcG8Q2xUrRonhit8QAtJ1g+pVivWu1VYbpLbK9HPQ8OZLl0bEOIE+XfsRjOsVkv"
"YrMKdG6I6HEFiUN9KDks8EiBRz03qVyEGG9B3xzPJujE1dBeYC8XVWpmj8CJqGmN4bT+Cm1UGP1Z0p/qAoTRmhSDuNH7xFvjUDLs"
"bWESpN01JlJ03T0gULg3HXbMKFA9Au1hJgXTlkHzB6I16ouKg3U2v1SnfiRMz0M06QgWNovjcyoBJtNB/nGAFnIIs6ZxNE27fZQO"
"1r45+TXa6NaeBIaQSy724lWGWpBMHY8ymzboCFv740SYFlkRcB+K839/VIBlzk7sVxnVp3UFiXizxczmIcMXC2ImPuqPWiHJh7Rd"
"g+yJvyrBqLArh0bhRrcZtOcPsQ9pQSbicwzLG+mgFaV96sqrnrJ4MugORvMMgZ0oR++vlx0oVBfqpSxtxPOyEXXrcaI14IG7yJYS"
"8YUgF21wa1tgljJ0/ByQD/YbSqdWh0c7fabonUXndNi2zFvq0MUMPF3pPOxwylBBmp+XC/O2SiQr/bh/qdLXtSz8p2suDunqap3s"
"Dou9k0dQJhxURRVOtuYw8bu8S/Yv1PRjD5HrHIo9TrUOE9tNQWg5Qi+4CiNu9uKWRRdlRFyQ5U07jp08xEZDhaeoeRIw20MzgmnF"
"JU0cYl8tCzXf0JoGfGZCeiLXDtYV8UdoBaZD3AWYxVCg70NGdzS7i6P63RxNMedQfa6TZKQ5glnhtnjn4wy6QEuvLxhw5aeMyEgT"
"+eBXeUeWpMydbVnQESV29MFD7AKkIiBzlisTv0Jtehf1LeG3PREWt5SBX5os4mTbcl2GA5HAypNaUR4eK/BTlCp6f/8P";


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

static String timeToString(long long time) {
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
    long long currentTime;
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
