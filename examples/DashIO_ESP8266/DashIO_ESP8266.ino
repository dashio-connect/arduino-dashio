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

const PROGMEM char *configStr = 
"zVjpjuo2FH4V5PsXTUkgw/KPXaNhEzBU1e38MMSANcHOdZw70NG8U5+hT9ZjJ4GQpeR2QZUQIif2Of6+s5oPdCDM91Dr6wc6zbhH"
"JeVsjuEbtSoP9UajjN6pLfdnUa2MJJUOidaiFlpOZ6iM6IazCT4QEMywjcJl8LQgUlK280DkYkGYfOqBtLeqGCCBTVJwR4vGWiLJ"
"UQa7fBcej6lDVawy2hO628uzyDCNz9cycvCaOHlYLMO6xmLkIfHkSR98OJ++zGJARkp9CoWZQDHqjEJkDhfw3P7m4ywcKRDVZk2B"
"UPg7/EgCHHHN3WVHa47ZhyefUQlr0R+/d2Nn7XKfSSJQ0koeZmW17dCdknT7k2V/DsItFwesnDGZTvrwnCIVziLIhnpaEzy9re3l"
"yVX226MRSoROJgcbh3vkeW1P2YIwG7Wk8Ili4YDdHEcWdGJExPhUGmP3duzNtOi2lyCM1PnWvpScDQX3886ZlTzmDydPe6PW3c6d"
"zjCePJ0h/N4Jaq8oeQ9Izcqkem4mBfACZHy77YaxPCd2wu5LEJLB+j7Da4fYkUHOon0zX7gAJxm6maEJr6bs15+m2y3KYysjJkLg"
"hWIuRvaEM4I+yzdADv8LkC/uHfCBkQS6oSCEpfGZt/F9adrNrbkpDLDH39kdIGozCZBfDLNasx7TMKsFYJpmo2FZhWGOyFbeAaY2"
"UyRSawUgDmrtar1bGOJcie+AMbCTANlx8OYtDdMqAHO9qRl2vTDMheT3yEltJgXSJyhVWQskZcROujukC/sFaf87LC4ONWhlOXBV"
"U0u3FusKcoc4DlJ9ZSewuw8bZvtIvTEFs0ZFcaYe9YTlLcL5azpJUPLUU33teN4I2wIt+Ihaj0pLejQ7nheAmau0/a6HuYtdhZ8c"
"XCKw9AXJZSfBa61hZQbHKa53TI8wAZfgrdKrX038QwcLoOIxPENckJhwUvQqKt8YXwdMRvPqZDoft0cJzp6D3uXBdDVlzmnK5sQh"
"2CNRKB1CarK4E8ReYRWZrXqMuWcwnOahWqnmMebt+Tv4SzshMJoxndcfA0iXGjWoVAYV2G5T7Ay44/B3T5sOdShxtPgXol6nIrOW"
"d3046OjRc+Y3KbucMbIJjvyB9tyTL/MRaLWxt39QX5Q/wF4YtT0iwojeCEx3kMX6GIEbYqOweblEzJ76T5NVwim94Irw49z3wFoO"
"95om6pw5iRp90RzXHnA5VdeGFK/MP6yJiGk53xHSzi3gA8u63FsGKO4PdWvyVRZ8VVehjXvtG5cLKEGWTmTqtm1bEE/pMJrmg/HY"
"eICPUWkqt2AHi0Ooxibf6YaoUTi8U0nhwHkH4JAF/Q1orVYUDnAB4PYPLAICoAOBp7PwyoHhBA6aOm87ZifruJJzYRMxPWu7yKLF"
"P++pjFanZOFzD4u3ErjyhC5BMKDCk6WeRlXSE37sJEuBmafDaXPShUi9WaqdcKmMrExh0Y6UwvkhtiB/94Uto5E9f/07tJp3oTVq"
"m+fWT+AY9v+GUzW2nwMXUmALtQbCH5jrxWO59qkQHMgw6qqvqtA7kC9cFLo9V/LKw4WXQBtKzlBxry1G82KXZ4XJcyj4Kmxc+ndi"
"trnW3Psb3Su3er6sUrXzH/Ss634Vlds1FmGzWmh00XaQ/3W3yuQPdkXT0KI/vKqUNhUk/HNAO56omW7Ed7n/f9WsZG1u3nI/qENZ"
"s2XcR/1RtvOz/qczDVMdHfs25Svq+UHrfP38Ew==";

DashioDevice dashioDevice(DEVICE_TYPE, configStr);
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
