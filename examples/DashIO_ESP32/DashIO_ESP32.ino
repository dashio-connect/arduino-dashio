#include "DashioESP.h"
#include "DashioProvisionESP.h"

#define DEVICE_TYPE "ESP32_DashIOM"
#define DEVICE_NAME "DashIO32_Master"

#define TCP_PORT 5650
#define MQTT_BUFFER_SIZE 4096

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

const PROGMEM char *configStr = 
"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";

DashDevice dashioDevice(DEVICE_TYPE, configStr, 1);
DashProvision dashioProvision(&dashioDevice);

// DashIO comms connections
    DashioWiFi wifi;
    DashTCP  tcp_con(&dashioDevice, TCP_PORT, true);
    DashMQTT mqtt_con(&dashioDevice, MQTT_BUFFER_SIZE, true, true);
    DashBLE  ble_con(&dashioDevice, true);

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
const char *DV03_ID = "DV03";

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

static DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    
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

char *ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
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
  message = dashioDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += dashioDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += dashioDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += dashioDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);

  message += dashioDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += dashioDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += dashioDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += dashioDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += dashioDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);

  sendMessage(connectionType, message);
  message = dashioDevice.getColorMessage(COL_ID, "#0080FF");
  message += dashioDevice.getAudioVisualMessage(AV_ID, F("http://192.168.68.170/mjpeg/1")); // URL for av feed (e.g. ESP32 Camera)

  int data1[] = {300, 270, 390, 410, 400};
  message += dashioDevice.getChartLineInts(CHART_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int));
  int data2[] = {160, 280, 400, 410, 420};
  message += dashioDevice.getChartLineInts(CHART_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int));
  int data3[] = {170, 290, 410, 400, 390};
  message += dashioDevice.getChartLineInts(CHART_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int));
  int data4[] = {180, 270, 390, 410, 430};
  message += dashioDevice.getChartLineInts(CHART_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int));
  int data5[] = {200, 250, 260, 265, 240};
  message += dashioDevice.getChartLineInts(CHART_ID, "L5", "Line Five", line, "8", data5, sizeof(data5)/sizeof(int));
    
  message += dashioDevice.getTimeGraphLine(TGRAPH_ID, "l1", "Roger", line, "purple");
  message += dashioDevice.getTimeGraphLine(TGRAPH_ID, "l2", "Betty", line, "white");

  message += dashioDevice.getMapTrackMessage(MAP_ID, "T3", "StatusX", "blue");

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
        mqtt_con.sendAlarmMessage(message);
    } else if (messageData->idStr == BUTTON02_ID) {
        String textArr[] = {"one", "two"};
        String message = dashioDevice.getEventLogMessage(LOG_ID, getUTCtime(), "red", textArr, 2);
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
            String message = dashioDevice.getDialMessage(DIAL_ID, messageData->payloadStr.toFloat());
            sendMessage(messageData->connectionType, message);
        }
        break;
    case timeGraph:
        break;
    case colorPicker:
        break;
    default:
        dashioProvision.processMessage(messageData);
        break;
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (commsChanged) {
        mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
        wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);
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
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);
    
    dashioDevice.setup(wifi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashioDevice.deviceID);

    wifi.setOnConnectCallback(&onWiFiConnectCallback);

    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin(); // 6 Digid PassKey for BLE

    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
    mqtt_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&mqtt_con);

    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);

    wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);

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

            // CReate something for TimeGraph control
            messageToSend += dashioDevice.getTimeGraphPoint(TGRAPH_ID, "l1", float(num));
            sawNum += 20;
            if (sawNum > 200) {
                sawNum = 0;
            }
            messageToSend += dashioDevice.getTimeGraphPoint(TGRAPH_ID, "l2", float(sawNum - 100));

            // Create something for EventLog control
            String lines[2];
            lines[0] = "Event Log Test";
            lines[1] = String(count);
            messageToSend += dashioDevice.getEventLogMessage(LOG_ID, getUTCtime(), "blue", lines, 2);
            
            // Create something for the Map control
            float lat = -43.559880 + (float)rNum / 10000;
            char latBuffer[16];
            sprintf(latBuffer, "%f", lat);
            
            float lon = 172.655620 + (float)random(-49, 50) / 10000;
            char lonBuffer[16];
            sprintf(lonBuffer, "%f", lon);

            messageToSend += dashioDevice.getMapWaypointMessage(MAP_ID, "T3", latBuffer, lonBuffer);
        }
    }
}
