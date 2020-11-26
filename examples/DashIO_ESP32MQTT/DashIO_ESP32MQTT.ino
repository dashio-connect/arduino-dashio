#include <WiFiClientSecure.h> // espressif library
#include <MQTT.h> // arduino-mqtt library created by Joël Gähwiler.
#include "DashIO.h"
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <EEPROM.h>

#define DEVICE_TYPE "SPARK32"
#define DEVICE_NAME "Spotty"

// WiFi
#define WIFI_SSID "yourWiFiSSID"
#define WIFI_PASSWORD "yourWiFiPassword"
#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes

// TCP
#define TCP_PORT 5000

// MQTT
#define MQTT_SERVER    "dash.dashio.io"
#define MQTT_PORT      8883
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"
#define MQTT_QOS       2
#define MQTT_RETRY_S   10 // Retry after 10 seconds

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

// Create WiFI and MQTT
WiFiClientSecure wifiClient;
MQTTClient mqttClient(1024); // Buffer size = 1024

// Create device
DashDevice myDevice;
  
// Create Connection
DashConnection myMQTTconnection(MQTT_CONN);

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
// Pages
const char *PAGE01_ID = "PG01";
const char *PAGE02_ID = "PG02";

// Page 1 Controls
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

// Page 2 Controls
const char *GRAPH_ID = "IDG";
const char *LABEL_ID = "LBL01";
const char *KNOB_ID = "KB01";
const char *DIAL_ID = "DL01";

// Hardware timer
hw_timer_t * timer = NULL;
bool oneSecond = false;    // Set by hardware timer every second. User for tasks that need to occur every second

int wifiConnectCount = 1;
int mqttConnectCount = 0;
String messageToSend = "";

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

// Hardware timer callback sets the oneSecond flag.
void IRAM_ATTR onTimerCallback() {
  oneSecond = true;
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
  if (!EEPROM.begin(EEPROM_SIZE)) {
      Serial.println(F("Failed to init EEPROM"));
  } else {
    EEPROMSetupObject userSetupRead;
    EEPROM.get(0, userSetupRead);
    if (userSetupRead.saved != 'Y') {
      saveUserSetup();
    } else {
      userSetup = userSetupRead;
    }

    Serial.print(F("Device Name: "));
    Serial.println(userSetupRead.deviceName);
    Serial.print(F("WiFi SSID: "));
    Serial.println(userSetupRead.wifiSSID);
    Serial.print(F("WiFi password: "));
    Serial.println(userSetupRead.wifiPassword);
    Serial.print(F("Dash userame: "));
    Serial.println(userSetupRead.dashUserName);
    Serial.print(F("Dash password: "));
    Serial.println(userSetupRead.dashPassword);
  }
}

void mqttPublish(String message, MQTTTopicType topic = data_topic) {
  if (mqttClient.connected()) {
    String publishTopic = myDevice.getMQTTTopic(MQTT_USER, topic);
    mqttClient.publish(publishTopic.c_str(), message.c_str(), false, MQTT_QOS);

    Serial.println(F("---- MQTT Sent ----"));
    Serial.print(F("Topic: "));
    Serial.println(publishTopic);
    Serial.print(F("Message: "));
    Serial.println(message);
  } else {
    Serial.println(F("Send Failed - MQTT not connected"));
  }
  Serial.println();
}

void sendAlarmMessage(String message) {
  mqttPublish(message, alarm_topic);
}

// Process incoming control mesages
void processStatus() {
  // Control initial condition messages (recommended)
  mqttPublish(myDevice.getDeviceNameMessage(userSetup.deviceName));

  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
  mqttPublish(message);

  message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += myDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += myDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += myDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += myDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);
  mqttPublish(message);

  int data1[] = {150, 270, 390, 410, 400};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int)));
  int data2[] = {160, 280, 400, 410, 420};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int)));
  int data3[] = {170, 290, 410, 400, 390};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int)));
  int data4[] = {180, 270, 390, 410, 430};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int)));
  int data5[] = {190, 280, 380, 370, 360};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L5", "Line Five", line, "7", data5, sizeof(data5)/sizeof(int)));
  int data6[] = {200, 250, 260, 265, 240};
  mqttPublish(myDevice.getGraphLineInts(GRAPH_ID, "L6", "Line Six", line, "8", data6, sizeof(data6)/sizeof(int)));
}

void processConfig() {
  Serial.print(F("Screen width: "));
  Serial.println(myMQTTconnection.idStr + "mm");
  Serial.print(F("Screen height: "));
  Serial.println(myMQTTconnection.payloadStr + "mm");

  mqttPublish(myDevice.getConfigMessage(DeviceCfg(2))); // Two pages

  EventLogCfg anEventLog(LOG_ID, PAGE01_ID, "Log", {0.05, 0.545, 0.9, 0.212});
  mqttPublish(myDevice.getConfigMessage(anEventLog));
  
  MapCfg aMap(MAP_ID, PAGE01_ID, "My Map", {0.0, 0.0, 1.0, 0.515});
  mqttPublish(myDevice.getConfigMessage(aMap));

  ButtonCfg aButton(BUTTON02_ID, PAGE01_ID, "Event", {0.5, 0.788, 0.2, 0.121});
  aButton.iconName = "bell";
  aButton.offColor = "blue";
  aButton.onColor = "black";
  mqttPublish(myDevice.getConfigMessage(aButton));

  MenuCfg aMenu(MENU_ID, PAGE01_ID, "Settings", {0.05, 0.788, 0.4, 0.121});
  aMenu.iconName = "pad";
  aMenu.text = "Setup";
  mqttPublish(myDevice.getConfigMessage(aMenu));

  SelectorCfg menuSelector(MENU_SELECTOR_ID, MENU_ID, "Selector");
  mqttPublish(myDevice.getConfigMessage(menuSelector));

  ButtonCfg menuButton(MENU_BUTTON_ID,  MENU_ID, "On/Off");
  menuButton.offColor = "red";
  menuButton.onColor = "purple";
  mqttPublish(myDevice.getConfigMessage(menuButton));

  TextBoxCfg menuTextBox(MENU_TEXTBOX_ID, MENU_ID, "Counter");
  menuTextBox.units = "°C";
  mqttPublish(myDevice.getConfigMessage(menuTextBox));

  SliderCfg menuSlider(MENU_SLIDER_ID, MENU_ID, "UV");
  menuSlider.knobColor = "green";
  menuSlider.barColor = "yellow";
  mqttPublish(myDevice.getConfigMessage(menuSlider));

  ButtonGroupCfg aGroup(BGROUP_ID, PAGE01_ID, "Actions", {0.75, 0.788, 0.2, 0.121});
  aGroup.iconName = "pad";
  aGroup.text = "BG";
  mqttPublish(myDevice.getConfigMessage(aGroup));

  ButtonCfg groupButton1(BGROUP_B01_ID, BGROUP_ID, "Up");
  groupButton1.iconName = "up";
  groupButton1.offColor = "red";
  groupButton1.onColor = "purple";
  mqttPublish(myDevice.getConfigMessage(groupButton1));

  ButtonCfg groupButton2(BGROUP_B02_ID, BGROUP_ID, "Down");
  groupButton2.iconName = "down";
  groupButton2.offColor = "Green";
  groupButton2.onColor = "#9d9f2c";
  mqttPublish(myDevice.getConfigMessage(groupButton2));

  ButtonCfg groupButton3(BGROUP_B03_ID, BGROUP_ID, "Left");
  groupButton3.iconName = "left";
  groupButton3.offColor = "#123456";
  groupButton3.onColor = "#228855";
  mqttPublish(myDevice.getConfigMessage(groupButton3));

  ButtonCfg groupButton4(BGROUP_B04_ID, BGROUP_ID, "Right");
  groupButton4.iconName = "right";
  groupButton4.offColor = "red";
  groupButton4.onColor = "#F4A37C";
  mqttPublish(myDevice.getConfigMessage(groupButton4));

  ButtonCfg groupButton5(BGROUP_B05_ID, BGROUP_ID, "Stop");
  groupButton5.iconName = "stop";
  groupButton5.offColor = "Black";
  groupButton5.onColor = "#bc41d7";
  mqttPublish(myDevice.getConfigMessage(groupButton5));

  GraphCfg aGraph(GRAPH_ID, PAGE02_ID, "Level", {0.0, 0.0, 1.0, 0.485});
  aGraph.xAxisLabel = "Temperature";
  aGraph.xAxisMin = 0;
  aGraph.xAxisMax = 1000;
  aGraph.xAxisNumBars = 6;
  aGraph.yAxisLabel = "Mixing Rate";
  aGraph.yAxisMin = 100;
  aGraph.yAxisMax = 600;
  aGraph.yAxisNumBars = 6;
  mqttPublish(myDevice.getConfigMessage(aGraph));

  LabelCfg aLabel(LABEL_ID, PAGE02_ID, "Label", {0.0, 0.515, 1.0, 0.394});
  aLabel.color = 22;
  mqttPublish(myDevice.getConfigMessage(aLabel));

  KnobCfg aKnob(KNOB_ID, PAGE02_ID, "Knob", {0.05, 0.576, 0.4, 0.303});
  aKnob.knobColor = "#FF00F0";
  aKnob.dialColor = "yellow";
  mqttPublish(myDevice.getConfigMessage(aKnob));

  DialCfg aDial(DIAL_ID, PAGE02_ID, "Dial", {0.55, 0.576, 0.4, 0.303});
  aDial.dialFillColor = "green";
  aDial.pointerColor = "yellow";
  aDial.style = dialInverted;
  aDial.units = "F";
  aDial.precision = 2;
  mqttPublish(myDevice.getConfigMessage(aDial));

  // Pages
  PageCfg pageOne(PAGE01_ID, "First Page", "down", "dark gray");
  pageOne.ctrlBkgndColor = "blue";
  pageOne.ctrlTitleBoxColor = 5;
  mqttPublish(myDevice.getConfigMessage(pageOne));
  
  PageCfg pageTwo(PAGE02_ID, "Second Page", "up", "0");
  pageTwo.ctrlBkgndColor = "blue";
  pageTwo.ctrlTitleBoxColor = 5;
  mqttPublish(myDevice.getConfigMessage(pageTwo));
}

void processButton() {
  if (myMQTTconnection.idStr == MENU_BUTTON_ID) {
    menuButtonValue = !menuButtonValue;
    String message = myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
    mqttPublish(message);
  } else if (myMQTTconnection.idStr == BGROUP_B01_ID) {
    buttonOneValue = !buttonOneValue;
    String message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
    mqttPublish(message);

    message = myDevice.getAlarmMessage("AL03", "An Alarm", "This is a test alarm");
    sendAlarmMessage(message);
  } else if (myMQTTconnection.idStr == BUTTON02_ID) {
    String localTimeStr = getLocalTime();
    String textArr[] = {"one", "two"};
    String message = myDevice.getEventLogMessage(LOG_ID, localTimeStr, "red", textArr, 2);
    mqttPublish(message);
  }
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
    case button:
      processButton();
      break;
    case textBox:
      if (myMQTTconnection.idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = myMQTTconnection.payloadStr.toFloat();
        String message = myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        mqttPublish(message);
      }
      break;
    case slider:
      if (myMQTTconnection.idStr == MENU_SLIDER_ID) {
        menuSliderValue = myMQTTconnection.payloadStr.toFloat();
        String message = myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        mqttPublish(message);
      }
      break;
    case selector:
      if (myMQTTconnection.idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = myMQTTconnection.payloadStr.toInt();
        String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        mqttPublish(message);
      }
      break;
    case knob:
      if (myMQTTconnection.idStr == KNOB_ID) {
        String message = myDevice.getDialMessage(DIAL_ID, myMQTTconnection.payloadStr);
        mqttPublish(message);
      }
      break;
    case timeGraph:
      break;
    case deviceName:
      if (myMQTTconnection.idStr != "") {
        myMQTTconnection.idStr.toCharArray(userSetup.deviceName, myMQTTconnection.idStr.length() + 1);
        saveUserSetup();
        Serial.print(F("Updated Device Name: "));
        Serial.println(userSetup.deviceName);
        mqttPublish(myDevice.getPopupMessage(F("Name Change Success"), "Changed to :" + myMQTTconnection.idStr, F("Happyness")));
      } else {
        mqttPublish(myDevice.getPopupMessage(F("Name Change Fail"), F("Fish"), F("chips")));
      }
      mqttPublish(myDevice.getDeviceNameMessage(userSetup.deviceName));
      break;
    case wifiSetup:
      myMQTTconnection.idStr.toCharArray(userSetup.wifiSSID, myMQTTconnection.idStr.length() + 1);
      myMQTTconnection.payloadStr.toCharArray(userSetup.wifiPassword, myMQTTconnection.payloadStr.length() + 1);
      saveUserSetup();
      Serial.print(F("Updated WIFI SSID: "));
      Serial.println(userSetup.wifiSSID);
      Serial.print(F("Updated WIFI Password: "));
      Serial.println(userSetup.wifiPassword);
      setupWiFiMQTT();
      break;
    case dashSetup:
      myMQTTconnection.idStr.toCharArray(userSetup.dashUserName, myMQTTconnection.idStr.length() + 1);
      myMQTTconnection.payloadStr.toCharArray(userSetup.dashPassword, myMQTTconnection.payloadStr.length() + 1);
      saveUserSetup();
      Serial.print(F("Updated Dash username: "));
      Serial.println(userSetup.dashUserName);
      Serial.print(F("Updated Dash Password: "));
      Serial.println(userSetup.dashPassword);
      setupWiFiMQTT();
      break;
  }
}

void checkIncomingMessages() {
  // Never process messages from within an interrupt.
  // Instead, look to see if the messageReceived flag is set for each connection.

  // Look for MQTT Messages
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
  Serial.print(F("Connecting to MQTT..."));
  if (mqttClient.connect(myDevice.deviceID.c_str(), userSetup.dashUserName, userSetup.dashPassword, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
    Serial.print(F("connected "));
    Serial.println(String(mqttClient.returnCode()));

    // Subscribe to private MQTT connection
    String subscriberTopic = myDevice.getMQTTSubscribeTopic(userSetup.dashUserName);
    mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS); // ... and subscribe
   
    // Send MQTT ONLINE and WHO messages to connection (Optional)
    // WHO is only required here if using the Dash server and it must be send to the ANNOUNCE topic
    mqttPublish(myDevice.getOnlineMessage());
    mqttPublish(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE), announce_topic); // Update announce topic with new name
  } else {
    Serial.print(F("Failed - Try again in 10 seconds: "));
    Serial.println(String(mqttClient.lastError()));
  }
}

void checkConnectivity() {
  // First, check WiFi and connect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnectCount++;
    Serial.print(F("Connecting to Wi-Fi "));
    Serial.println(String(wifiConnectCount));
    if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP32. Sometimes ESP32's WiFi gets tied up in a knot.
      ESP.restart();
    }
  } else {
    // WiFi OK
    if (wifiConnectCount > 0) {
        wifiConnectCount = 0; // So that we only execute the following once after WiFI connection
      
        Serial.print("Connected with IP: ");
        Serial.println(WiFi.localIP());

        configTime(0, 0, ntpServer); // utcOffset = 0 and daylightSavingsOffset = 0, so time is UTC
        time_t now;
        long currentTime;
        time(&currentTime);
        Serial.print(F("NTP Time: "));
        Serial.println(timeToString(currentTime));
    }
    
    // Check and connect MQTT as necessary
    if (!mqttClient.connected()) {      
      if (mqttConnectCount == 0) {
        mqttHostConnect();
      }
      if (mqttConnectCount >= MQTT_RETRY_S) {
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
  String willTopic = myDevice.getMQTTTopic(MQTT_USER, will_topic);
  Serial.print(F("LWT topic: "));  
  Serial.println(willTopic);

  String offlineMessage = myDevice.getOfflineMessage();
  mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);

  Serial.print("LWT message: ");
  Serial.println(offlineMessage);
}

void setupWiFiMQTT() {
  // Below is required to get WiFi to connect reliably on ESP32
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin(userSetup.wifiSSID, userSetup.wifiPassword);
  wifiConnectCount = 1;

  // Setup MQTT host
  mqttClient.begin(MQTT_SERVER, MQTT_PORT, wifiClient);
  mqttClient.setOptions(10, true, 10000);  // 10s timeout
  mqttClient.onMessageAdvanced(messageReceivedMQTTCallback);
  
  setupMQTT_LWT(); // Once the deviceID is known
}
  
void setupTimer() {
  // Setup the hardware timer
  timer = timerBegin(0, 80, true); // Use 1st timer of 4. 1 tick takes 1/(80MHZ/80) = 1us so we set divider 80 and count up
  timerAttachInterrupt(timer, &onTimerCallback, true); //Attach callback function, onTimerCallback, to the timer
  timerAlarmWrite(timer, 1000000, true); // 1 tick is 1us => 1s and we want 1s.
  timerAlarmEnable(timer);
}

void setup() {
  Serial.begin(115200);

  loadUserSetup();

  // Get unique deviceID
  myDevice.setDeviceID(WiFi.macAddress());
  Serial.print(F("Device ID: "));
  Serial.println(myDevice.deviceID);

  setupWiFiMQTT();
  setupTimer();
}

void loop() {
  if (oneSecond) { // Tasks to occur every second
    oneSecond = false;
    checkConnectivity(); // Only check once a second. Therefore counters are based on 1s intervals.

    messageToSend = myDevice.getMapMessage(MAP_ID, "-43.559880", "172.655620", "12345");
  }

  checkIncomingMessages();
  checkOutgoingMessages();
}
