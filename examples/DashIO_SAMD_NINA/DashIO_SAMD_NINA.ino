#include "DashioSAMD_NINA.h"
#include "DashioProvisionSAMD.h"

#include <arduino-timer.h>

#define NO_TCP
//#define NO_MQTT

#define DEVICE_TYPE "SAMD_NINA_Type"
#define DEVICE_NAME "SAMD_NINA_DashIO"

#define TCP_PORT 5650

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

// BLE
#define MIN_BLE_TIME_S 10
  
const PROGMEM char *configC64Str = 
"lVXdbuI4FH6VyteACJDwc1cog6phoALa0Wi1F05yklhNbMZ2Btiq77TPME82x04CBOiqKyEUn5zfL9/5/EYy4Lkio7/+bpCU+pCW"
"zxr2eiz2YI5vJBBcS5E+PpAReXzYjEmDbKkErq3ladZ20JJzptGd4KNmOgV8nIica5BoSYDFiV5RzQQZtVtOxym9noRiaOTovVk+"
"kaLyfcpiY5lMF5vpCo2RkBnVaFksF1M8H6qwY8Zep4dNSQiYstnaDfLqh5vD1vRxP59j0I6FOjkG9N0G2V+l6aA1SIWCr3645Gvg"
"IRlpmcM7YpLRbYmOn2st+EyKvG4p0BJRNBGpkFh4nOaAlev4GfgK/ymnfgpliQYR/BRHg9ebKF8BeUJ77JCPQP0AL4N18cXq2HRu"
"QNMgDMdY0MyUWkEkQSXEwBJLuk2KyQ/3e6a+MazrtDFgb45zy6q1Ptgel4srOGak9LSBGFZkoXsyGpgs1xjsjw5Ypn0CYA6/ICXn"
"dQ0AkG1BUp1L+BCeC1C7Xu8WZG7fK5urcn9je8bjO/Qwue2rRZ6NqUQ43LKPo8Gro+zcwNjAiYSYCM4hKDp8IxJo+Pxs5/fBd3uD"
"LjS7HjjNnjcYNP1+5DaBtvtdz/E7Hh1gIwrkLxZAGdSLaBR02k7TiXy32XOH0BxEQdAM3GAYdLvO0On5hgGSafg/dd5xw7jwywXI"
"fmpd7zsRSj+v5pgupCppmT8mWjgnSgV2WDLJpAkZrXQnwAXIZSVCwbaecisk8tX1XMPG7X0YIgmN5DjDTsvxBi38OZ5nUtKUyqxK"
"wzKY3SJp+5KKmxk5p597k32XZBn2Tgysk63Gld//Tj6/npdk8j6rXoZBIZivj8hFLDao8Tx7sKYXBjtM5rxXPqXBKLyWKc78BeFY"
"s39wlC72oBIcHiUpz3glUZirMCjL4HP4SnhMpvFrzMNLDTR2IUOQy2O2k61y/p4wXXlf2crzIw9ZLM6uGftx755oDOf1N5JyZT9f"
"cDBSUrzamBi82arkS/SK4W6FKlx3qIe75+EnlByvJosPYsetJipIkbWiIrJKGc5YIF08Xyh/nYZru8McL6D0sOQrzEUVVK5ZqXu3"
"qCkhfKEG7lH/TBafX/6LtVdkVInY4XrYDShK3qCn2fwKwpkE4OZOo/KLSFOxU2s7IxlFuNdgX1S+P8A4fPLCwbjq3lhPzWpmdmst"
"x5ksZKFEGJWf67mIyyPNQyZemMpLYXn/Aw==";

DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);
DashioProvision dashioProvision(&dashioDevice);

// DashIO comms connections
DashioWiFi wifi;
    DashioBLE  ble_con(&dashioDevice, true);
#ifndef NO_TCP
    DashioTCP  tcp_con(&dashioDevice, TCP_PORT, true);
#endif
#ifndef NO_MQTT
    DashioMQTT mqtt_con(&dashioDevice, true, true);
#endif

// variables for button
const int buttonPin = 2;

// Create Control IDs
const char *DV01_ID = "PG01";
const char *TIME_GRAPH_ID = "IDTG";
const char *SLIDER_ID = "IDS";
const char *BUTTON_ID = "IDB";
const char *TEXTBOX_ID = "IDTB";
const char *GRAPH_ID = "IDG";

auto timer = timer_create_default();
bool oneSecond = false; // Set by hardware timer every second.
ButtonMultiState toggle = off;
unsigned int bleTimer = MIN_BLE_TIME_S; // Start off in WiFi mode
bool bleActive = true; // Start off in WiFi mode

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == BLE_CONN) {
        ble_con.sendMessage(message);
    } else if (connectionType == TCP_CONN) {
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
    // Control initial condition messages (recommended)
    String message = dashioDevice.getButtonMessage(BUTTON_ID, toggle);
    message += dashioDevice.getSingleBarMessage(SLIDER_ID, 25);
    int data[] = {100, 200, 300, 400, 500};
    message += dashioDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1", peakBar, "purple", data, sizeof(data)/sizeof(int));
    sendMessage(connectionType, message);
    
    int data2[] = {100, 200, 300, 250, 225, 280, 350, 500, 550, 525, 500};
    message = dashioDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2", line, "aqua", data2, sizeof(data2)/sizeof(int));
    sendMessage(connectionType, message);
}

void processConfig(ConnectionType connectionType) {
    sendMessage(connectionType, dashioDevice.getConfigMessage(DeviceCfg(1, "name, wifi, dashio"))); // One device view
    
    TimeGraphCfg timpGraphCfg(TIME_GRAPH_ID, DV01_ID, "Temperature", {0.25, 0.0, 0.75, 0.394});
    timpGraphCfg.yAxisLabel = "°C";
    timpGraphCfg.yAxisMin = 0;
    timpGraphCfg.yAxisMax = 50;
    sendMessage(connectionType, dashioDevice.getConfigMessage(timpGraphCfg));
    
    SliderCfg sliderCfg(SLIDER_ID, DV01_ID, "UV", {0.0, 0.0, 0.2, 0.394});
    sliderCfg.knobColor = "green";
    sliderCfg.barFollowsSlider = false;
    sliderCfg.barColor = "yellow";
    sendMessage(connectionType, dashioDevice.getConfigMessage(sliderCfg));
    
    ButtonCfg buttonCfg(BUTTON_ID, DV01_ID, "B1", {0.0, 0.424, 0.2, 0.12});
    buttonCfg.iconName = "refresh";
    buttonCfg.offColor = "blue";
    buttonCfg.onColor = "black";
    sendMessage(connectionType, dashioDevice.getConfigMessage(buttonCfg));
    
    TextBoxCfg textBoxCfg(TEXTBOX_ID, DV01_ID, "Counter", {0.25, 0.424, 0.75, 0.1212});
    sendMessage(connectionType, dashioDevice.getConfigMessage(textBoxCfg));
    
    GraphCfg graphCfg(GRAPH_ID,  DV01_ID, "Level", {0.0, 0.576, 1.0, 0.364});
    graphCfg.xAxisLabel = "Temperature";
    graphCfg.xAxisMin = 0;
    graphCfg.xAxisMax = 1000;
    graphCfg.xAxisNumBars = 6;
    graphCfg.yAxisLabel = "Mixing Rate";
    graphCfg.yAxisMin = 100;
    graphCfg.yAxisMax = 800;
    graphCfg.yAxisNumBars = 5;
    sendMessage(connectionType, dashioDevice.getConfigMessage(graphCfg));
  
    // Connections
    BLEConnCfg bleCnctnConfig(SERVICE_UUID, CHARACTERISTIC_UUID, CHARACTERISTIC_UUID);
    sendMessage(connectionType, dashioDevice.getConfigMessage(bleCnctnConfig));

#ifndef NO_TCP
    TCPConnCfg tcpCnctnConfig(wifi.ipAddress(), TCP_PORT);
    sendMessage(connectionType, dashioDevice.getConfigMessage(tcpCnctnConfig));
#endif

#ifndef NO_MQTT
    MQTTConnCfg mqttCnctnConfig(dashioProvision.dashUserName, DASH_SERVER);
    sendMessage(connectionType, dashioDevice.getConfigMessage(mqttCnctnConfig));
#endif
    
    DeviceViewCfg deviceViewCfg(DV01_ID, "Graph Page", "down", "93");
    deviceViewCfg.ctrlBkgndColor = "blue";
    deviceViewCfg.ctrlBkgndTransparency = 80;
    deviceViewCfg.ctrlTitleFontSize = 16;
    deviceViewCfg.ctrlTitleBoxColor = "5";
    deviceViewCfg.ctrlTitleBoxTransparency = 50;
    sendMessage(connectionType, dashioDevice.getConfigMessage(deviceViewCfg));
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == BUTTON_ID) {
        if (toggle == off) {
            toggle = on;
            int data[] = {50, 255, 505, 758, 903, 400, 0};
            String message = dashioDevice.getGraphLineInts(GRAPH_ID, "L1", "Line 1 a", bar, "green", data, sizeof(data)/sizeof(int));
            int data2[] = {153, 351, 806, 900, 200, 0};
            message += dashioDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 a", segBar, "yellow", data2, sizeof(data2)/sizeof(int));
            sendMessage(messageData->connectionType, message);
        } else if (toggle == on) {
            toggle = off;
            float data[] = {200, 303.334, 504.433, 809.434, 912, 706, 643};
            String message = dashioDevice.getGraphLineFloats(GRAPH_ID, "L1", "Line 1 b", peakBar, "orange", data, sizeof(data)/sizeof(float));
            int data2[] = {};
            message += dashioDevice.getGraphLineInts(GRAPH_ID, "L2", "Line 2 b", peakBar, "blue", data2, sizeof(data2)/sizeof(int));
            sendMessage(messageData->connectionType, message);
        }
        
        String message = dashioDevice.getButtonMessage(BUTTON_ID, toggle);
        int data[] = {25, 75};
        message += dashioDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]);
        sendMessage(messageData->connectionType, message);
    }
}

void processSlider(MessageData *messageData) {
    String payload = messageData->payloadStr;
    int data[2];
    data[0] = 100 - payload.toInt();
    data[1] = payload.toInt()/2;
    sendMessage(messageData->connectionType, dashioDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]));
}

void processTimeGraph(MessageData *messageData) {
/*???
  long startTime = currentTime - (60 * 5);
    
  String timeStrArr[] = {timeToString(startTime),
                         timeToString(startTime + (60 * 1)),
                         timeToString(startTime + (60 * 2)),
                         timeToString(startTime + (60 * 3)),
                         timeToString(startTime + (60 * 4)),
                         timeToString(startTime + (60 * 5))};

  float data3[] = {20, 30, 35, 38, 40, 35};
  String message = dashioDevice.getTimeGraphLineFloats(TIME_GRAPH_ID, "L1", "Line 1", line, "aqua", timeStrArr, data3, sizeof(data3)/sizeof(float), true);

  sendMessage(messageData->connectionType, message);
*/
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData->connectionType);
        break;
    case config:
        processConfig(messageData->connectionType);
        break;
    case button:
        processButton(messageData);
        break;
    case slider:
        processSlider(messageData);
        break;
    case timeGraph:
Serial.println(messageData->payloadStr);//???
        processTimeGraph(messageData);
        break;
    default:
        dashioProvision.processMessage(messageData);
        break;
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if ((!bleActive) && (commsChanged)) {
        bleTimer = MIN_BLE_TIME_S; // Force reconnect of WiFi
        bleActive = true;
    }
}

// Timer Interrupt
static bool onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

void startBLE() {
    Serial.println("Startup BLE");
    bleActive = true;
    wifi.end();

    bleTimer = 0;
    ble_con.begin();
}

void startWiFi() {
    Serial.println("Startup WiFi");
    bleActive = false;
    ble_con.end();

#ifndef NO_MQTT
    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword); // Setup MQTT host
#endif
    while (!wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword, 0)) {
        if (digitalRead(buttonPin) == HIGH) {
            break;
        }
    }
}

void setup() {  
    // Start the Serial port
    Serial.begin(115200);
    delay(2000);
  
    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashioDevice.setup(wifi.macAddress());  // device type, unique deviceID, and device name
    Serial.print(F("Device ID: "));
    Serial.println(dashioDevice.deviceID);

    ble_con.setCallback(&processIncomingMessage);
#ifndef NO_TCP
    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);
#endif

#ifndef NO_MQTT
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword); // Setup MQTT host
    wifi.attachConnection(&mqtt_con);
#endif

    startWiFi();

    timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    timer.tick();

    if (bleActive) {
        ble_con.run();
        
        if (oneSecond) { // Tasks to occur every second
            oneSecond = false;
            Serial.print("BLE Timer: ");
            Serial.println(bleTimer);
            if ((bleTimer >= MIN_BLE_TIME_S) && (ble_con.connected() == false)) {
                startWiFi();
            }
            bleTimer += 1;
        }
    } else {
        int buttonState = digitalRead(buttonPin); // read the button pin    
        if (buttonState) {
            if (!ble_con.connected()) {
                startBLE();
            }
        }

        if (!bleActive && !wifi.run()) { // WiFi has dropped off
            Serial.println("WiFi disconnected");
            startWiFi();
        }
    }
}
