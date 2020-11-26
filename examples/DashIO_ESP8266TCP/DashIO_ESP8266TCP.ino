#include <WiFiClientSecure.h> // espressif library
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "DashIO.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <timer.h>
#include <TimeLib.h>
#include <EEPROM.h>

#define DEVICE_TYPE "ADA8266"
#define DEVICE_NAME "Rabid Dogs"

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

// Create WiFi and TCP clients
WiFiServer wifiServer(TCP_PORT);
WiFiClient client;
WiFiClientSecure wifiClientSecure; // Required for SSL

// Create device
DashDevice myDevice;
  
// Create Connection(s)
DashConnection myTCPconnection(TCP_CONN);

// Create Control IDs
const char *PAGE01_ID = "PG01";
const char *TIME_GRAPH_ID = "IDTG";
const char *SLIDER_ID = "IDS";
const char *BUTTON_ID = "IDB";
const char *TEXT_BOX_ID = "IDTB";
const char *GRAPH_ID = "IDG";

auto timer = timer_create_default();
bool oneSecond = false;    // Set by timer every second. Used for tasks that need to occur every second
int wifiConnectCount = 0;
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
}

void tcpWriteStr(String message) {
    if (client.connected()) {
        Serial.println("**** TCP Sent ****");
        Serial.println(message);
        Serial.println();
        
        client.print(message);
    }
}

// Process incoming control mesages
void processStatus() {
    // Control initial condition messages (recommended)
    tcpWriteStr(myDevice.getDeviceNameMessage(userSetup.deviceName));
    
    String message = myDevice.getButtonMessage(BUTTON_ID, toggle);
    message += myDevice.getSingleBarMessage(SLIDER_ID, 25);
    int data[] = {100, 200, 300, 400, 500};
    message += myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1", peakBar, "purple", data, sizeof(data)/sizeof(int));
    tcpWriteStr(message);
    
    int data2[] = {100, 200, 300, 250, 225, 280, 350, 500, 550, 525, 500};
    message = myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2", line, "aqua", data2, sizeof(data2)/sizeof(int));
    tcpWriteStr(message);
}

void processConfig() {
    tcpWriteStr(myDevice.getConfigMessage(DeviceCfg(1))); // One page
    
    TimeGraphCfg timpGraphCfg(TIME_GRAPH_ID, PAGE01_ID, "Temperature", {0.25, 0.0, 0.75, 0.394});
    timpGraphCfg.yAxisLabel = "°C";
    timpGraphCfg.yAxisMin = 0;
    timpGraphCfg.yAxisMax = 50;
    tcpWriteStr(myDevice.getConfigMessage(timpGraphCfg));
    
    SliderCfg sliderCfg(SLIDER_ID, PAGE01_ID, "UV", {0.0, 0.0, 0.2, 0.394});
    sliderCfg.knobColor = "green";
    sliderCfg.barFollowsSlider = false;
    sliderCfg.barColor = "yellow";
    tcpWriteStr(myDevice.getConfigMessage(sliderCfg));
    
    ButtonCfg buttonCfg(BUTTON_ID, PAGE01_ID, "B1", {0.0, 0.424, 0.2, 0.12});
    buttonCfg.iconName = "refresh";
    buttonCfg.offColor = "blue";
    buttonCfg.onColor = "black";
    tcpWriteStr(myDevice.getConfigMessage(buttonCfg));
    
    TextBoxCfg textBoxCfg(TEXT_BOX_ID, PAGE01_ID, "Counter", {0.25, 0.424, 0.75, 0.1212});
    tcpWriteStr(myDevice.getConfigMessage(textBoxCfg));
    
    GraphCfg graphCfg(GRAPH_ID,  PAGE01_ID, "Level", {0.0, 0.576, 1.0, 0.364});
    graphCfg.xAxisLabel = "Temperature";
    graphCfg.xAxisMin = 0;
    graphCfg.xAxisMax = 1000;
    graphCfg.xAxisNumBars = 6;
    graphCfg.yAxisLabel = "Mixing Rate";
    graphCfg.yAxisMin = 100;
    graphCfg.yAxisMax = 800;
    graphCfg.yAxisNumBars = 5;
    tcpWriteStr(myDevice.getConfigMessage(graphCfg));
    
    PageCfg pageCfg(PAGE01_ID, "Graph Page", "down", "93");
    pageCfg.ctrlBkgndColor = "blue";
    pageCfg.ctrlBkgndTransparency = 80;
    pageCfg.ctrlTitleFontSize = 16;
    pageCfg.ctrlTitleBoxColor = "5";
    pageCfg.ctrlTitleBoxTransparency = 50;
    tcpWriteStr(myDevice.getConfigMessage(pageCfg));
}

void processButton() {
  if (myTCPconnection.idStr == BUTTON_ID) {
    if (toggle == off) {
      toggle = on;
      int data[] = {50, 255, 505, 758, 903, 400, 0};
      String message = myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1 a", bar, "green", data, sizeof(data)/sizeof(int));
      int data2[] = {153, 351, 806, 900, 200, 0};
      message += myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 a", segBar, "yellow", data2, sizeof(data2)/sizeof(int));
      tcpWriteStr(message);
    } else if (toggle == on) {
      toggle = off;
      float data[] = {200, 303.334, 504.433, 809.434, 912, 706, 643};
      String message = myDevice.getGraphLineFloats(GRAPH_ID, "L1", "Line 1 b", peakBar, "orange", data, sizeof(data)/sizeof(float));
      int data2[] = {};
      message += myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 b", peakBar, "blue", data2, sizeof(data2)/sizeof(int));
      tcpWriteStr(message);
    }

    String message = myDevice.getButtonMessage(BUTTON_ID, toggle);
    int data[] = {25, 75};
    message += myDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]);
    tcpWriteStr(message);
  }
}

void processSlider() {
  String payload = myTCPconnection.payloadStr;
  int data[2];
  data[0] = 100 - payload.toInt();
  data[1] = payload.toInt()/2;
  tcpWriteStr(myDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]));
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

  tcpWriteStr(message);
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
        case deviceName:
            if (myTCPconnection.idStr != "") {
                myTCPconnection.idStr.toCharArray(userSetup.deviceName, myTCPconnection.idStr.length() + 1);
                saveUserSetup();
                Serial.print("Updated Device Name: ");
                Serial.println(userSetup.deviceName);
                tcpWriteStr(myDevice.getPopupMessage("Name Change Success", "Changed to :" + myTCPconnection.idStr, "Happyness"));
            } else {
                tcpWriteStr(myDevice.getPopupMessage("Name Change Fail", "Fish", "chips"));
            }
            tcpWriteStr(myDevice.getDeviceNameMessage(userSetup.deviceName));
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
    // First, check WiFi and connect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount++;
        Serial.println("Connecting to Wi-Fi " + String(wifiConnectCount));
        if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP8266. Sometimes ESP's WiFi gets tied up in a knot.
            ESP.restart();
        }
    } else {
    // If WiFi OK
        if (wifiConnectCount > 0) {
            wifiConnectCount = 0; // So that we only execute the following once after WiFI connection
        
            Serial.print("Connected with IP: ");
            Serial.println(WiFi.localIP());
    
            if (!MDNS.begin("esp8266")) {
                Serial.println("Error setting up mDNS");
            }
            Serial.println("mDNS started");
            MDNS.addService("DashIO", "tcp", TCP_PORT); // Announce esp tcp service
        }
    }
}

void setupWiFi() {
  WiFi.begin(userSetup.wifiSSID, userSetup.wifiPassword);
  wifiClientSecure.setInsecure(); // For SSL
  wifiServer.begin(); // For TCP
  wifiConnectCount = 1;
}

void setup() {
  Serial.begin(115200);

  loadUserSetup();
  
  myDevice.setDeviceID(WiFi.macAddress());
  Serial.print("Device ID: ");
  Serial.println(myDevice.deviceID);

  setupWiFi();

  timeClient.begin();
  timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    MDNS.update();
    timer.tick();
    if (oneSecond) {
        oneSecond = false;
        checkConnectivity();
    
        timeClient.update();
        currentTime = timeClient.getEpochTime();
        messageToSend = myDevice.getTextBoxMessage(TEXT_BOX_ID, timeClient.getFormattedTime());
    }
    
    checkIncomingMessages();
    checkOutgoingMessages();
}
