#include "DashioESP.h"
#include "DashioProvisionESP.h"

#define DEVICE_TYPE "ESP32_Dash"
#define DEVICE_NAME "Dash32_Master"

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

const PROGMEM char *configStr = 
"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"
"/devvwE=";

DashDevice dashDevice(DEVICE_TYPE, configStr, 4);
DashProvision dashProvision(&dashDevice);

// dash comms connections
    DashWiFi wifi;
    DashTCP  tcp_con(&dashDevice, true);
    DashMQTT mqtt_con(&dashDevice, true, true);
    DashBLE  ble_con(&dashDevice, true);

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
const char *CHART_ID = "IDG";
const char *TGRAPH_ID = "TG01";
const char *LABEL_ID = "LBL01";
const char *KNOB_ID = "KB01";
const char *DIAL_ID = "DL01";

// Device View 3 Controls
const char *COL_ID = "IDC01";
const char *AV_ID = "AV01";

const char* ntpServer = "pool.ntp.org";
bool oneSecond = false; // Set by hardware timer every second.
int count = 0;
String messageToSend = ((char *)0);
int walk = 5;
int sawNum = 0;
    
String getUTCtime() {
  struct tm dt; // dateTime
  if(!getLocalTime(&dt)){
    Serial.println(F("Failed to obtain time"));
    return "";
  }
  char buffer[22];
  sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", 1900 + dt.tm_year, 1 + dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec); // Z = UTC time
  String dateTime(buffer);
  return dateTime;
}

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == TCP_CONN) {
        tcp_con.sendMessage(message);
    } else if (connectionType == BLE_CONN) {
        ble_con.sendMessage(message);
    } else {
        mqtt_con.sendMessage(message);
    }
}

void sendMessageAll(const String& message) {
    tcp_con.sendMessage(message);
    ble_con.sendMessage(message);
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

  message += dashDevice.getButtonMessage(BUTTON02_ID, true);
  
  message += dashDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += dashDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += dashDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += dashDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += dashDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);

  sendMessage(connectionType, message);
  message = dashDevice.getColorMessage(COL_ID, "#0080FF");
  message += dashDevice.getAudioVisualMessage(AV_ID, F("http://192.168.68.170/mjpeg/1")); // URL for av feed (e.g. ESP32 Camera)

  int data1[] = {300, 270, 390, 410, 400};
  dashDevice.addChartLineInts(message, CHART_ID, "L1", "Line One", line, "3", yLeft, data1, sizeof(data1)/sizeof(int));
  int data2[] = {160, 280, 400, 410, 420};
  dashDevice.addChartLineInts(message, CHART_ID, "L2", "Line Two", line, "4", yLeft, data2, sizeof(data2)/sizeof(int));
  int data3[] = {170, 290, 410, 400, 390};
  dashDevice.addChartLineInts(message, CHART_ID, "L3", "Line Three", line, "5", yLeft, data3, sizeof(data3)/sizeof(int));
  int data4[] = {180, 270, 390, 410, 430};
  dashDevice.addChartLineInts(message, CHART_ID, "L4", "Line Four", line, "6", yLeft, data4, sizeof(data4)/sizeof(int));
  int data5[] = {200, 250, 260, 265, 240};
  dashDevice.addChartLineInts(message, CHART_ID, "L5", "Line Five", line, "8", yLeft, data5, sizeof(data5)/sizeof(int));
    
  message += dashDevice.getTimeGraphLine(TGRAPH_ID, "l1", "Roger", line, "purple", yLeft);
  message += dashDevice.getTimeGraphLine(TGRAPH_ID, "l2", "Betty", line, "white", yLeft);

  message += dashDevice.getMapTrackMessage(MAP_ID, "T3", "StatusX", "blue");

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
        sendMessageAll(message); // Send to all to make sure it get captured by the dash server.
    }
}

void onWiFiConnectCallback(void) {
    configTime(0, 0, ntpServer); // utcOffset = 0 and daylightSavingsOffset = 0, so time is UTC
    Serial.print(F("NTP UTC Time: "));
    Serial.println(getUTCtime());
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
            String message = dashDevice.getDialMessage(DIAL_ID, messageData->payloadStr.toFloat());
            sendMessage(messageData->connectionType, message);
        }
        break;
    case timeGraph:
        break;
    case colorPicker:
        break;
    default:
        dashProvision.processMessage(messageData);
        break;
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (commsChanged) {
        mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
        wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);
    } else {
        mqtt_con.sendWhoAnnounce(); // Update announce topic with new name
    }
}

void checkOutgoingMessages() {
    if (messageToSend.length() > 0) {
        ble_con.sendMessage(messageToSend);
        tcp_con.sendMessage(messageToSend);
        mqtt_con.sendMessage(messageToSend);
        messageToSend = "";
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
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);
    
    dashDevice.setup(wifi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashDevice.deviceID);

    wifi.setOnConnectCallback(&onWiFiConnectCallback);

    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin(); // 6 Digid PassKey for BLE

    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.addDashStore(timeGraph, TGRAPH_ID);
    mqtt_con.addDashStore(eventLog, LOG_ID);
    
    wifi.attachConnection(&mqtt_con);

    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);

    wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);

    // Setup 1 second timer task
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
    ble_con.run();
    wifi.run();

    checkOutgoingMessages();

    if (oneSecond) { // Tasks to occur every second
        oneSecond = false;
        count++;
        if (count > 60000) {
            count = 0;
        }
        if (count % 60 == 0) {
            messageToSend = "";
            
            long rNum = random(-49, 50);
            int num = walk + rNum;
            if (num >= 100) {
                num = 200 - num;
            }   
            if (num <= -100) {
                num = -200 - num;
            }

            // Create something for TimeGraph control
            messageToSend += dashDevice.getTimeGraphPoint(TGRAPH_ID, "l1", float(num));
            sawNum += 20;
            if (sawNum > 200) {
                sawNum = 0;
            }
            messageToSend += dashDevice.getTimeGraphPoint(TGRAPH_ID, "l2", float(sawNum - 100));

            // Create something for EventLog control
            String lines[2];
            lines[0] = "Event Log Test";
            lines[1] = String(count);
            dashDevice.addEventLogMessage(messageToSend, LOG_ID, getUTCtime(), "blue", lines, 2);
            
            // Create something for the Map control
            float lat = -43.559880 + (float)rNum / 10000;
            char latBuffer[16];
            sprintf(latBuffer, "%f", lat);
            
            float lon = 172.655620 + (float)random(-49, 50) / 10000;
            char lonBuffer[16];
            sprintf(lonBuffer, "%f", lon);

            messageToSend += dashDevice.getMapWaypointMessage(MAP_ID, "T3", latBuffer, lonBuffer);
        }
    }
}
