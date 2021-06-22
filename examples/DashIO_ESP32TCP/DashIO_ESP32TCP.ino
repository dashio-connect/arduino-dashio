#include <WiFiClientSecure.h> // espressif library
#include <ESPmDNS.h>
#include "DashIO.h"
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <EEPROM.h>

#define DEVICE_TYPE "SPARK32"
#define DEVICE_NAME "Multipass"

// WiFi
#define WIFI_SSID "yourWiFiSSID"
#define WIFI_PASSWORD "yourWiFiPassword"
#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes

// TCP
#define TCP_PORT 5050

#define EEPROM_SIZE 244

struct EEPROMSetupObject {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char saved;
};

EEPROMSetupObject userSetup = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, 'Y'};

// Create TCP
WiFiServer wifiServer(TCP_PORT);
WiFiClient client;

// Create device
DashDevice myDevice;
  
// Create Connections
DashConnection myTCPconnection(TCP_CONN);

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

// Hardware timer
hw_timer_t * timer = NULL;
bool oneSecond = false;    // Set by hardware timer every second. User for tasks that need to occur every second

int wifiConnectCount = 0;
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
  }
}

void tcpWriteStr(String message) {
  if (client.connected()) {
    Serial.println(F("**** TCP Sent ****"));
    Serial.println(message);
    Serial.println();

    client.print(message);
  }
}

// Process incoming control mesages
void processStatus() {
  // Control initial condition messages (recommended)
  tcpWriteStr(myDevice.getDeviceNameMessage(userSetup.deviceName));

  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
  tcpWriteStr(message);

  message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += myDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += myDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += myDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += myDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);
  tcpWriteStr(message);

  int data1[] = {150, 270, 390, 410, 400};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int)));
  int data2[] = {160, 280, 400, 410, 420};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int)));
  int data3[] = {170, 290, 410, 400, 390};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int)));
  int data4[] = {180, 270, 390, 410, 430};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int)));
  int data5[] = {190, 280, 380, 370, 360};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L5", "Line Five", line, "7", data5, sizeof(data5)/sizeof(int)));
  int data6[] = {200, 250, 260, 265, 240};
  tcpWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L6", "Line Six", line, "8", data6, sizeof(data6)/sizeof(int)));
}

void processConfig() {
  Serial.print(F("Screen width: "));
  Serial.println(myTCPconnection.idStr + "mm");
  Serial.print(F("Screen height: "));
  Serial.println(myTCPconnection.payloadStr + "mm");
  
  tcpWriteStr(myDevice.getConfigMessage(DeviceCfg(2))); // Two Device Views

  EventLogCfg anEventLog(LOG_ID, DV01_ID, "Log", {0.05, 0.545, 0.9, 0.212});
  tcpWriteStr(myDevice.getConfigMessage(anEventLog));
  
  MapCfg aMap(MAP_ID, DV01_ID, "My Map", {0.0, 0.0, 1.0, 0.515});
  tcpWriteStr(myDevice.getConfigMessage(aMap));

  ButtonCfg aButton(BUTTON02_ID, DV01_ID, "Event", {0.5, 0.788, 0.2, 0.121});
  aButton.iconName = "bell";
  aButton.offColor = "blue";
  aButton.onColor = "black";
  tcpWriteStr(myDevice.getConfigMessage(aButton));

  MenuCfg aMenu(MENU_ID, DV01_ID, "Settings", {0.05, 0.788, 0.4, 0.121});
  aMenu.iconName = "pad";
  aMenu.text = "Setup";
  tcpWriteStr(myDevice.getConfigMessage(aMenu));

  SelectorCfg menuSelector(MENU_SELECTOR_ID, MENU_ID, "Selector");
  tcpWriteStr(myDevice.getConfigMessage(menuSelector));

  ButtonCfg menuButton(MENU_BUTTON_ID,  MENU_ID, "On/Off");
  menuButton.offColor = "red";
  menuButton.onColor = "purple";
  tcpWriteStr(myDevice.getConfigMessage(menuButton));

  TextBoxCfg menuTextBox(MENU_TEXTBOX_ID, MENU_ID, "Counter");
  menuTextBox.units = "°C";
  tcpWriteStr(myDevice.getConfigMessage(menuTextBox));

  SliderCfg menuSlider(MENU_SLIDER_ID, MENU_ID, "UV");
  menuSlider.knobColor = "green";
  menuSlider.barColor = "yellow";
  tcpWriteStr(myDevice.getConfigMessage(menuSlider));

  ButtonGroupCfg aGroup(BGROUP_ID, DV01_ID, "Actions", {0.75, 0.788, 0.2, 0.121});
  aGroup.iconName = "pad";
  aGroup.text = "BG";
  tcpWriteStr(myDevice.getConfigMessage(aGroup));

  ButtonCfg groupButton1(BGROUP_B01_ID, BGROUP_ID, "Up");
  groupButton1.iconName = "up";
  groupButton1.offColor = "red";
  groupButton1.onColor = "purple";
  tcpWriteStr(myDevice.getConfigMessage(groupButton1));

  ButtonCfg groupButton2(BGROUP_B02_ID, BGROUP_ID, "Down");
  groupButton2.iconName = "down";
  groupButton2.offColor = "Green";
  groupButton2.onColor = "#9d9f2c";
  tcpWriteStr(myDevice.getConfigMessage(groupButton2));

  ButtonCfg groupButton3(BGROUP_B03_ID, BGROUP_ID, "Left");
  groupButton3.iconName = "left";
  groupButton3.offColor = "#123456";
  groupButton3.onColor = "#228855";
  tcpWriteStr(myDevice.getConfigMessage(groupButton3));

  ButtonCfg groupButton4(BGROUP_B04_ID, BGROUP_ID, "Right");
  groupButton4.iconName = "right";
  groupButton4.offColor = "red";
  groupButton4.onColor = "#F4A37C";
  tcpWriteStr(myDevice.getConfigMessage(groupButton4));

  ButtonCfg groupButton5(BGROUP_B05_ID, BGROUP_ID, "Stop");
  groupButton5.iconName = "stop";
  groupButton5.offColor = "Black";
  groupButton5.onColor = "#bc41d7";
  tcpWriteStr(myDevice.getConfigMessage(groupButton5));

  GraphCfg aGraph(GRAPH_ID, DV02_ID, "Level", {0.0, 0.0, 1.0, 0.485});
  aGraph.xAxisLabel = "Temperature";
  aGraph.xAxisMin = 0;
  aGraph.xAxisMax = 1000;
  aGraph.xAxisNumBars = 6;
  aGraph.yAxisLabel = "Mixing Rate";
  aGraph.yAxisMin = 100;
  aGraph.yAxisMax = 600;
  aGraph.yAxisNumBars = 6;
  tcpWriteStr(myDevice.getConfigMessage(aGraph));

  LabelCfg aLabel(LABEL_ID, DV02_ID, "Label", {0.0, 0.515, 1.0, 0.394});
  aLabel.color = 22;
  tcpWriteStr(myDevice.getConfigMessage(aLabel));

  KnobCfg aKnob(KNOB_ID, DV02_ID, "Knob", {0.05, 0.576, 0.4, 0.303});
  aKnob.knobColor = "#FF00F0";
  aKnob.dialColor = "yellow";
  tcpWriteStr(myDevice.getConfigMessage(aKnob));

  DialCfg aDial(DIAL_ID, DV02_ID, "Dial", {0.55, 0.576, 0.4, 0.303});
  aDial.dialFillColor = "green";
  aDial.pointerColor = "yellow";
  aDial.style = dialPieInverted;
  aDial.units = "F";
  aDial.precision = 2;
  tcpWriteStr(myDevice.getConfigMessage(aDial));

  // Device Views
  DeviceViewCfg deviceViewOne(DV01_ID, "First Device View", "down", "dark gray");
  deviceViewOne.ctrlBkgndColor = "blue";
  deviceViewOne.ctrlTitleBoxColor = 5;
  tcpWriteStr(myDevice.getConfigMessage(deviceViewOne));
  
  DeviceViewCfg deviceViewTwo(DV02_ID, "Second Device View", "up", "0");
  deviceViewTwo.ctrlBkgndColor = "blue";
  deviceViewTwo.ctrlTitleBoxColor = 5;
  tcpWriteStr(myDevice.getConfigMessage(deviceViewTwo));
}

void processButton() {
  if (myTCPconnection.idStr == MENU_BUTTON_ID) {
    menuButtonValue = !menuButtonValue;
    String message = myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
    tcpWriteStr(message);
  } else if (myTCPconnection.idStr == BGROUP_B01_ID) {
    buttonOneValue = !buttonOneValue;
    String message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
    tcpWriteStr(message);
  } else if (myTCPconnection.idStr == BUTTON02_ID) {
    String localTimeStr = getLocalTime();
    String textArr[] = {"one", "two"};
    String message = myDevice.getEventLogMessage(LOG_ID, localTimeStr, "red", textArr, 2);
    tcpWriteStr(message);
  }
}

void processIncomingMessage() {
  switch (myTCPconnection.control) {
    case who:
      tcpWriteStr(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE));
      break;
    case connect:
      tcpWriteStr(myDevice.getConnectMessage());
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
      if (myTCPconnection.idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = myTCPconnection.payloadStr.toFloat();
        String message = myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        tcpWriteStr(message);
      }
      break;
    case slider:
      if (myTCPconnection.idStr == MENU_SLIDER_ID) {
        menuSliderValue = myTCPconnection.payloadStr.toFloat();
        String message = myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        tcpWriteStr(message);
      }
      break;
    case selector:
      if (myTCPconnection.idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = myTCPconnection.payloadStr.toInt();
        String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        tcpWriteStr(message);
      }
      break;
    case knob:
      if (myTCPconnection.idStr == KNOB_ID) {
        String message = myDevice.getDialMessage(DIAL_ID, myTCPconnection.payloadStr);
        tcpWriteStr(message);
      }
      break;
    case timeGraph:
      break;
    case deviceName:
      if (myTCPconnection.idStr != "") {
        myTCPconnection.idStr.toCharArray(userSetup.deviceName, myTCPconnection.idStr.length() + 1);
        saveUserSetup();
        Serial.print(F("Updated Device Name: "));
        Serial.println(userSetup.deviceName);
        tcpWriteStr(myDevice.getPopupMessage(F("Name Change Success"), "Changed to :" + myTCPconnection.idStr, F("Happyness")));
      } else {
        tcpWriteStr(myDevice.getPopupMessage(F("Name Change Fail"), F("Fish"), F("chips")));
      }
      tcpWriteStr(myDevice.getDeviceNameMessage(userSetup.deviceName));
      break;
    case wifiSetup:
      myTCPconnection.idStr.toCharArray(userSetup.wifiSSID, myTCPconnection.idStr.length() + 1);
      myTCPconnection.payloadStr.toCharArray(userSetup.wifiPassword, myTCPconnection.payloadStr.length() + 1);
      saveUserSetup();
      Serial.print(F("Updated WIFI SSID: "));
      Serial.println(userSetup.wifiSSID);
      Serial.print(F("Updated WIFI Password: "));
      Serial.println(userSetup.wifiPassword);
      setupWiFi();
      break;
  }
}

void checkIncomingMessages() {
  // Look for TCP messages
  if (!client) {
    client = wifiServer.available();
    client.setTimeout(2000);
  } else {
    if (client.connected()) {
      while (client.available()>0) {
        char c = client.read();
        if (myTCPconnection.processChar(c)) {
          Serial.println("**** TCP Received ****");
          Serial.print(myTCPconnection.deviceID);
          Serial.print("   " + myDevice.getControlTypeStr(myTCPconnection.control));
          Serial.println("   " + myTCPconnection.idStr + "   " + myTCPconnection.payloadStr);

          processIncomingMessage();
        }
      }
    } else {
      client.stop();
      client = wifiServer.available();
      client.setTimeout(2000);
    }
  }
}

void checkOutgoingMessages() {
  // Never send messages from within an interrupt.
  // Instead, look to see if there is a meassge ready to send in messageToSend.
  if (messageToSend.length() > 0) {
    tcpWriteStr(messageToSend);
    messageToSend = "";
  }
}

void checkConnectivity() {
  // First, check WiFi and connect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Connecting to Wi-Fi "));
    Serial.println(String(wifiConnectCount));
    wifiConnectCount++;
    if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP32. Sometimes ESP32's WiFi gets tied up in a knot.
      ESP.restart();
    }
  } else {
    // WiFi OK
    if (wifiConnectCount > 0) {
      wifiConnectCount = 0; // So that we only execute the following once after WiFI connection

      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
  
      if(!MDNS.begin("esp32")) {
         Serial.println("Error starting mDNS");
         return;
      }
      Serial.println("mDNS started");
      MDNS.addService("DashIO", "tcp", TCP_PORT);
  
      configTime(0, 0, ntpServer); // utcOffset = 0 and daylightSavingsOffset = 0, so time is UTC
      time_t now;
      long currentTime;
      time(&currentTime);
      Serial.print(F("NTP Time: "));
      Serial.println(timeToString(currentTime));
    }
  }
}

void setupWiFi() {
  // Below is required to get WiFi to connect reliably on ESP32
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin(userSetup.wifiSSID, userSetup.wifiPassword);
  wifiServer.begin(); // For TCP
  wifiConnectCount = 1;
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
  myDevice.deviceID = WiFi.macAddress();
  Serial.print(F("Device ID: "));
  Serial.println(myDevice.deviceID);

  setupWiFi();
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
