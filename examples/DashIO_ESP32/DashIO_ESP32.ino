#include "DashioESP.h"
#include "DashioProvisionESP.h"
#include <WiFiUdp.h>
#include <TimeLib.h>

//#define NO_BLE
#define NO_TCP
//#define NO_MQTT

#define DEVICE_TYPE "ESP32_DashIO"
#define DEVICE_NAME "DashIO32"

#define TCP_PORT 5000
#define MQTT_BUFFER_SIZE 2048

// WiFi
#define WIFI_SSID "yourWiFiSSID"
#define WIFI_PASSWORD "yourWiFiPassword"

// MQTT
#define MQTT_USER "yourMQTTuserName"
#define MQTT_PASSWORD "yourMQTTpassword"

const char configC64Str[] PROGMEM =
"zVjrbuo4EH6Vyudv6SYh4dJ/JARUlQKC0tXq7PlhiAGrIeY4Tgtb9Z32GfbJduwkEHLZ5uylWqlC9WDP+JtvPBfe0I4EUYhuv76h"
"45SFVFAWzDB8olvtpt3pXKNX6ontSWReI0GFT9K96BY9TqboGtEVC8Z4R0AwxR5KtsFqToSgwSYE0R5zEoi7Pkj7T5oOEjgkOPOV"
"6EFJBDmI+FS0h+WhcCnNukZbQjdbcRLphv7+7Rr5eEn8KiyWbl1i0auQhOKoLj6cTRbTDJCRVF9AYeRQjOxRgsxnHNa97xEuw1EA"
"0eyaEoTEb7MDiXFkNTuPttKcsQ+rKKAC9qI/fncyd3VYFAjCUd5KFWZptefTjZQ47vjRnYFwzfgOSzLGk7EL64JT4S6crGioNMHq"
"eek9HvfSfm80QrnQKfXBymchuV96k2BOAg/dCh4R6YUd3lcQWZPE1BEPx6sHvP849qZK9DFLEEbyfstICBYMOYuq7ln2eIwffjy9"
"ldz38duxh9nHYw/h/w2n3hMlr7FTy15Su/IlxfBiZGy9dpJYnhEvZ3cRh2S83w3w0ideapAF6blpxPcAJx+6paEJX02CX3+arNeo"
"ylslMZEArxVzGWePWUDQ+/UHIIf/BcjF/hPwgZEcuiEnJCjiMz7G96XrddfGqjbAPnsNPgGiMpMD+UU3mqbVKsJs1oBpGJ2OZdWG"
"OSJr8QkwlZk6kWrWgDgwe822UxviTIo/AWNsJwfS9vHquQjTqgFzuTJ1r10b5lywz3iTykwBZERQIbPWeJSpd4rVoZjYz0jdF9hc"
"H2pcyirgyqJWLC3WBWSb+D6SdWXD8X6bFMzegYYPFMzqmvSZXKoOK5wn/ddknHPJXV/WtcPpIByLteADum1JLcXW7HDaAGYunu2L"
"aubOdiV+stsTjkXESaV3cn41O1ZpcByzeh/oATrgK/hW6lVfjaOdjTm4opXcISvIdTgF96oS7ROHBQFZxbd7Q5xgb7GIuwHXtsxO"
"0200W67eMFudTsNuD6yG29PazZZuG61eBy4SEv5CVyQ5ZA56A8fQ9IY+sK2GaXXdRmfgOA3HcrpOs6l3ddOW5HMqyI/YeYe+MGDL"
"mPW0tx5PZg+9UY7f+7jOhtAJTgL/OAlmxCc4JGnY7xIay3jmxHvC8hXdtjMs34PhImdNrVnFbrhlrxBbKmBioyWTRLsVQzrn04Gm"
"DTQ47lHsD5jvs9dQmU50SHG6+Rcivy68IrNq1NmpSFc98XchLinfslAsZiPQ6uFweyM/KLuBszAWALnJ65MMyBvEDGQ6duM860zv"
"3LvxU46PfjzJ/Ljb+2Ctwu3KQ9Q/uSPtR+qmIuX8PaNyuim4NIh2S8IzWk6jTJHXGu63rPN4NUBZKuRwF8nH+lVObKv9JS17xiFT"
"Wirf0H3P8zgJpQ69a9zorc4N/OlaW9KCfcx3qRq6I8M0P8LaI/JxguY13UitAK6vRLKphz3me7onEchxUXAfMA6AxDn9DahoahI7"
"0Aa+inZBCh50xYJQJZgL0pPhAjTZz5vAy5coKWfcI3xy0naWpZt/3lKR7i7IknUf8+croP+IzoEzoDwUVzHMKzW8ZG7yyHEQqhBc"
"HVWOld88ypMwL6dWJrBpQ66S1iizofr02Vt6p7y1/HfcanyKW9OO4NTVELiG97/xqZxIvskc78N7YbzWkK9VpYczxlgbyrd6WQbm"
"o1m9GV/dz6fg96Rmqf9zLdil5v7fKFyV2XPxVMid/6BcXZaqNN0uMU/q1FyhS4+D/K8LVan/4FTatM3d4UWm9CgnyW8YKqsR2XqO"
"2KbyZzrTyufm7kf0gzpU1gJnOXJH5eSX/Zxo6Ia8Oo48yp5oGMWl89v7nw==";


DashioDevice    dashioDevice(DEVICE_TYPE, configC64Str);
DashioProvision dashioProvision(&dashioDevice);

// DashIO comms connections
    DashioWiFi wifi;
#ifndef NO_TCP
    DashioTCP  tcp_con(&dashioDevice, TCP_PORT, true);
#endif
#ifndef NO_MQTT
    DashioMQTT mqtt_con(&dashioDevice, MQTT_BUFFER_SIZE, true, true);
#endif
#ifndef NO_BLE
    DashioBLE  ble_con(&dashioDevice, true);
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
bool oneSecond = false; // Set by timer every second.
int count = 0;
String messageToSend = ((char *)0);

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

char *ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == TCP_CONN) {
#ifndef NO_TCP
        tcp_con.sendMessage(message);
#endif
    } else if (connectionType == BLE_CONN) {
#ifndef NO_BLE
        ble_con.sendMessage(message);
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

void processButton(MessageData *connection) {
    if (connection->idStr == MENU_BUTTON_ID) {
        menuButtonValue = !menuButtonValue;
        String message = dashioDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
        sendMessage(connection->connectionType, message);
    } else if (connection->idStr == BGROUP_B01_ID) {
        buttonOneValue = !buttonOneValue;
        String message = dashioDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
        sendMessage(connection->connectionType, message);

        message = dashioDevice.getAlarmMessage("AL03", "An Alarm", "This is a test alarm");
#ifndef NO_MQTT
        mqtt_con.sendAlarmMessage(message);
#endif
    } else if (connection->idStr == BUTTON02_ID) {
        String localTimeStr = getLocalTime();
        String textArr[] = {"one", "two"};
        String message = dashioDevice.getEventLogMessage(LOG_ID, localTimeStr, "red", textArr, 2);
        sendMessage(connection->connectionType, message);
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

void processIncomingMessage(MessageData *connection) {
    switch (connection->control) {
    case status:
        processStatus(connection->connectionType);
        break;
    case button:
        processButton(connection);
        break;
    case textBox:
        if (connection->idStr == MENU_TEXTBOX_ID) {
            menuTextBoxValue = connection->payloadStr.toFloat();
            String message = dashioDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
            sendMessage(connection->connectionType, message);
        }
        break;
    case slider:
        if (connection->idStr == MENU_SLIDER_ID) {
            menuSliderValue = connection->payloadStr.toFloat();
            String message = dashioDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
            sendMessage(connection->connectionType, message);
        }
        break;
    case selector:
        if (connection->idStr == MENU_SELECTOR_ID) {
            menuSelectorIndex = connection->payloadStr.toInt();
            String message = dashioDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
            sendMessage(connection->connectionType, message);
        }
        break;
    case knob:
        if (connection->idStr == KNOB_ID) {
            String message = dashioDevice.getDialMessage(DIAL_ID, connection->payloadStr.toFloat());
            sendMessage(connection->connectionType, message);
        }
        break;
    case timeGraph:
        break;
    default:
        dashioProvision.processMessage(connection);
        break;
    }
}

void checkOutgoingMessages() {
    if (messageToSend.length() > 0) {
#ifndef NO_BLE
        ble_con.sendMessage(messageToSend);
#endif
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
#ifndef NO_MQTT
        mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
#endif
        wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);
    }
}

void oneSecondTimerTask(void *parameters) {
    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        oneSecond = true;
    }
}

void setup() {
    Serial.begin(115200);

    messageToSend.reserve(200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashioDevice.setup(wifi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashioDevice.deviceID);
  
#ifndef NO_BLE
    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin(true);
#endif
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

    // Setup 1 second timer task for checking connectivity
    xTaskCreate(
        oneSecondTimerTask,
        "One Second",
        1024, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );
}

void loop() {
    wifi.run();

#ifndef NO_BLE
    ble_con.run();
#endif

    checkOutgoingMessages();

    if (oneSecond) { // Tasks to occur every second
        oneSecond = false;
        count++;
        if (count > 64000) {
            count = 0;
        }
        if (count % 5 == 0) {
            messageToSend = dashioDevice.getMapWaypointMessage(MAP_ID, "TX1", "-43.603488", "172.649536");
        }
    }
}
