#include <WiFiClientSecure.h> // espressif library
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include "DashIO.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <timer.h>
#include <TimeLib.h>
#include <EEPROM.h>

#define DEVICE_TYPE "ADA8266"
#define DEVICE_NAME "Rabid Dog"

// WiFi
#define WIFI_SSID "yourWiFiSSID"
#define WIFI_PASSWORD "yourWiFiPassword"
#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes

// MQTT
#define MQTT_SERVER      "dash.dashio.io"
#define MQTT_PORT        8883 // 8883 for SSL
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"
#define MQTT_QOS         2
#define MQTT_TIMEOUT_S   10 // Retry after 10 seconds

#define EEPROM_SIZE 244

struct EEPROMSetupObject {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];
    char saved;
};

EEPROMSetupObject userSetup = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD, 'Y'};

// Create WiFi and MQTT clients
WiFiClientSecure wifiClient;
MQTTClient mqttClient(1024); // Buffer size = 1024

// Create device
DashDevice myDevice;
  
// Create Connection(s)
DashConnection myMQTTconnection(MQTT_CONN);

// Create Control IDs
const char *DV01_ID = "DV01";
const char *TIME_GRAPH_ID = "IDTG";
const char *SLIDER_ID = "IDS";
const char *BUTTON_ID = "IDB";
const char *TEXTBOX_ID = "IDTB";
const char *GRAPH_ID = "IDG";

auto timer = timer_create_default();
bool oneSecond = false;    // Set by timer every second. Used for tasks that need to occur every second
int wifiConnectCount = 0;
int mqttConnectCount = 0;
String messageToSend = "";
ButtonMultiState toggle = off;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0); // utcOffset = 0, so time is UTC
long currentTime;

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

long createTimeElements(const char *str) {
  int Year, Month, Day, Hour, Minute, Second;
  tmElements_t tm;
    
  sscanf(str, "%d-%d-%dT%d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
  tm.Year = CalendarYrToTm(Year);
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
  return makeTime(tm);
}

// Timer Interrupt
static bool onTimerCallback(void *argument) {
  oneSecond = true;
  return true; // to repeat the timer action - false to stop
}
  
// MQTT callback for when a message is received
static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length) {
  myMQTTconnection.processMessage(String(payload)); // The message components are stored within the connection where the messageReceived flag is set
}
    
void saveUserSetup() {
  EEPROM.put(0, userSetup);
  EEPROM.commit();
}

void loadUserSetup() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROMSetupObject userSetupRead;
  EEPROM.get(0, userSetupRead);
  if (userSetupRead.saved != 'Y') {
    saveUserSetup();
  } else {
    userSetup = userSetupRead;
  }

  Serial.print("Device Name: ");
  Serial.println(userSetupRead.deviceName);
  Serial.print("WiFi SSID: ");
  Serial.println(userSetupRead.wifiSSID);
  Serial.print("WiFi password: ");
  Serial.println(userSetupRead.wifiPassword);
  Serial.print("Dash userame: ");
  Serial.println(userSetupRead.dashUserName);
  Serial.print("Dash password: ");
  Serial.println(userSetupRead.dashPassword);
}

static void mqttPublish(String message, MQTTTopicType topic = data_topic) {
    if (mqttClient.connected()) {
        String publishTopic = myDevice.getMQTTTopic(userSetup.dashUserName, topic);
        mqttClient.publish(publishTopic.c_str(), message.c_str(), false, MQTT_QOS);
    
        Serial.println("---- MQTT Sent ----");
        Serial.println("Topic: " + publishTopic);
        Serial.println("Message: " + message);
    } else {
        Serial.println("Send Failed - MQTT not connected");
    }
    Serial.println();
}

// Process incoming control mesages
void processStatus() {
    // Control initial condition messages (recommended)
    String message = myDevice.getButtonMessage(BUTTON_ID, toggle);
    message += myDevice.getSingleBarMessage(SLIDER_ID, 25);
    int data[] = {100, 200, 300, 400, 500};
    message += myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1", peakBar, "purple", data, sizeof(data)/sizeof(int));
    mqttPublish(message);

    int data2[] = {100, 200, 300, 250, 225, 280, 350, 500, 550, 525, 500};
    message = myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2", line, "aqua", data2, sizeof(data2)/sizeof(int));
    mqttPublish(message);
}

void processConfig() {
    mqttPublish(myDevice.getConfigMessage(DeviceCfg(1))); // One device view
    
    TimeGraphCfg timpGraphCfg(TIME_GRAPH_ID, DV01_ID, "Temperature", {0.25, 0.0, 0.75, 0.394});
    timpGraphCfg.yAxisLabel = "°C";
    timpGraphCfg.yAxisMin = 0;
    timpGraphCfg.yAxisMax = 50;
    mqttPublish(myDevice.getConfigMessage(timpGraphCfg));
    
    SliderCfg sliderCfg(SLIDER_ID, DV01_ID, "UV", {0.0, 0.0, 0.2, 0.394});
    sliderCfg.knobColor = "green";
    sliderCfg.barFollowsSlider = false;
    sliderCfg.barColor = "yellow";
    mqttPublish(myDevice.getConfigMessage(sliderCfg));
    
    ButtonCfg buttonCfg(BUTTON_ID, DV01_ID, "B1", {0.0, 0.424, 0.2, 0.12});
    buttonCfg.iconName = "refresh";
    buttonCfg.offColor = "blue";
    buttonCfg.onColor = "black";
    mqttPublish(myDevice.getConfigMessage(buttonCfg));
    
    TextBoxCfg textBoxCfg(TEXTBOX_ID, DV01_ID, "Counter", {0.25, 0.424, 0.75, 0.1212});
    mqttPublish(myDevice.getConfigMessage(textBoxCfg));
    
    GraphCfg graphCfg(GRAPH_ID,  DV01_ID, "Level", {0.0, 0.576, 1.0, 0.364});
    graphCfg.xAxisLabel = "Temperature";
    graphCfg.xAxisMin = 0;
    graphCfg.xAxisMax = 1000;
    graphCfg.xAxisNumBars = 6;
    graphCfg.yAxisLabel = "Mixing Rate";
    graphCfg.yAxisMin = 100;
    graphCfg.yAxisMax = 800;
    graphCfg.yAxisNumBars = 5;
    mqttPublish(myDevice.getConfigMessage(graphCfg));
    
    DeviceViewCfg deviceViewCfg(DV01_ID, "Graph Device View", "down", "93");
    deviceViewCfg.ctrlBkgndColor = "blue";
    deviceViewCfg.ctrlBkgndTransparency = 80;
    deviceViewCfg.ctrlTitleFontSize = 16;
    deviceViewCfg.ctrlTitleBoxColor = "5";
    deviceViewCfg.ctrlTitleBoxTransparency = 50;
    mqttPublish(myDevice.getConfigMessage(deviceViewCfg));
}

void processButton() {
  if (myMQTTconnection.idStr == BUTTON_ID) {
    if (toggle == off) {
      toggle = on;
      int data[] = {50, 255, 505, 758, 903, 400, 0};
      String message = myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1 a", bar, "green", data, sizeof(data)/sizeof(int));
      int data2[] = {153, 351, 806, 900, 200, 0};
      message += myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 a", segBar, "yellow", data2, sizeof(data2)/sizeof(int));
      mqttPublish(message);
    } else if (toggle == on) {
      toggle = off;
      float data[] = {200, 303.334, 504.433, 809.434, 912, 706, 643};
      String message = myDevice.getGraphLineFloats(GRAPH_ID, "L1", "Line 1 b", peakBar, "orange", data, sizeof(data)/sizeof(float));
      int data2[] = {};
      message += myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 b", peakBar, "blue", data2, sizeof(data2)/sizeof(int));
      mqttPublish(message);
    }

    String message = myDevice.getButtonMessage(BUTTON_ID, toggle);
    int data[] = {25, 75};
    message += myDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]);
    mqttPublish(message);
  }
}

void processSlider() {
  String payload = myMQTTconnection.payloadStr;
  int data[2];
  data[0] = 100 - payload.toInt();
  data[1] = payload.toInt()/2;
  mqttPublish(myDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]));
}

void processTimeGraph() {
  long startTime = currentTime - (60 * 5);
    
  String timeStrArr[] = {timeToString(startTime),
                         timeToString(startTime + (60 * 1)),
                         timeToString(startTime + (60 * 2)),
                         timeToString(startTime + (60 * 3)),
                         timeToString(startTime + (60 * 4)),
                         timeToString(startTime + (60 * 5))};

  float data3[] = {20, 30, 35, 38, 40, 35};
  String message = myDevice.getTimeGraphLineFloats(TIME_GRAPH_ID, "L1", "Line 1", line, "aqua", timeStrArr, data3, sizeof(data3)/sizeof(float));

  mqttPublish(message);
}

void processIncomingMessage() {
    switch (myMQTTconnection.control) {
        case who:
            mqttPublish(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE));
            break;
        case connect:
            mqttPublish(myDevice.getConnectMessage());
            break;
        case status:
            processStatus();
            break;
        case config:
            processConfig();
            break;
        case deviceName:
            if (myMQTTconnection.idStr != "") {
                myMQTTconnection.idStr.toCharArray(userSetup.deviceName, myMQTTconnection.idStr.length() + 1);
                saveUserSetup();
                Serial.print("Updated Device Name: ");
                Serial.println(userSetup.deviceName);
                mqttPublish(myDevice.getPopupMessage("Name Change Success", "Changed to :" + myMQTTconnection.idStr, "Happyness"));
            } else {
                mqttPublish(myDevice.getPopupMessage("Name Change Fail", "Fish", "chips"));
            }
            mqttPublish(myDevice.getDeviceNameMessage(userSetup.deviceName));
            break;
        case button:
            processButton();
            break;
        case slider:
            processSlider();
            break;
        case timeGraph:
            processTimeGraph();
            break;
    }
}

void checkIncomingMessages() {
    // Never process messages from within an interrupt.
    // Instead, look to see if the messageReceived flag is set for each connection.

    if (mqttClient.connected()) {
        mqttClient.loop();
           if (myMQTTconnection.messageReceived) {
               myMQTTconnection.messageReceived = false;
               processIncomingMessage();
        }
    }
}

void checkOutgoingMessages() {
    // Never send messages from within an interrupt.
    // Instead, look to see if there is a meassge ready to send in messageToSend.
    if (messageToSend.length() > 0) {
        mqttPublish(messageToSend);
        messageToSend = "";
    }
}

void mqttHostConnect() { // Non-blocking
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect(myDevice.deviceID.c_str(), userSetup.dashUserName, userSetup.dashPassword, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
        Serial.println("connected " + String(mqttClient.returnCode()));
        Serial.println();

        // Subscribe to private MQTT connection
        String subscriberTopic = myDevice.getMQTTSubscribeTopic(userSetup.dashUserName);
        mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS); // ... and subscribe
   
        // Send MQTT ONLINE and WHO messages to connection (Optional)
        // WHO is only required here is using the Dash server and it must be send to the ANNOUNCE topic
        mqttPublish(myDevice.getOnlineMessage());
        mqttPublish(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE), announce_topic); // Update announce topic with new name
    } else {
        Serial.println("Failed, " + String(mqttClient.lastError()) + ". Try again in 10 seconds");
    }
}

void checkConnectivity() {
    // First, check WiFi and connect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount++;
        Serial.println("Connecting to Wi-Fi " + String(wifiConnectCount));
        if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP8266. Sometimes ESP's WiFi gets tied up in a knot.
            ESP.restart();
        }
    } else {
        // WiFi OK
        if (wifiConnectCount > 0) {
            wifiConnectCount = 0; // So that we only execute the following once after WiFI connection

            Serial.print("Connected with IP: ");
            Serial.println(WiFi.localIP());
        }

        // Check MQTT and connect as necessary
        if (!mqttClient.connected()) {
            wifiConnectCount = 0;
      
            if (mqttConnectCount == 0) {
                mqttHostConnect();
            }
            if (mqttConnectCount >= MQTT_TIMEOUT_S) {
                mqttConnectCount = 0;
            } else {
                mqttConnectCount++;
            }
        } else {
            mqttConnectCount = 0;
        }
    }
}

void setupMQTT_LWT() {
  // Setup MQTT Last Will and Testament message (Optional). Default keep alive time is 10s
  String willTopic = myDevice.getMQTTTopic(userSetup.dashUserName, will_topic);
  Serial.print("LWT topic: ");
  Serial.println(willTopic);
  
  String offlineMessage = myDevice.getOfflineMessage();
  mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);
  
  Serial.print("LWT message: ");
  Serial.println(offlineMessage);
}

void setupWiFiMQTT() {
  WiFi.begin(userSetup.wifiSSID, userSetup.wifiPassword);
  wifiClient.setInsecure(); // For SSL
  wifiConnectCount = 1;

  // Setup MQTT host
  mqttClient.begin(MQTT_SERVER, MQTT_PORT, wifiClient);
  mqttClient.setOptions(10, true, 10000); // 10s timeout
  mqttClient.onMessageAdvanced(messageReceivedMQTTCallback);
  setupMQTT_LWT();
}

void setup() {
  Serial.begin(115200);

  loadUserSetup();
  
  myDevice.setDeviceID(WiFi.macAddress());
  Serial.print("Device ID: ");
  Serial.println(myDevice.deviceID);

  setupWiFiMQTT();

  timeClient.begin();
  timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
  timer.tick();
  if (oneSecond) {
    oneSecond = false;
    checkConnectivity();
  
    timeClient.update();
    currentTime = timeClient.getEpochTime();
    messageToSend = myDevice.getTextBoxMessage(TEXTBOX_ID, timeClient.getFormattedTime());
  }
  
  checkIncomingMessages();
  checkOutgoingMessages();
}
