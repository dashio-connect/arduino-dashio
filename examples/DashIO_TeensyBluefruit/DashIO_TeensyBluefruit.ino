// Teensy 3.6 with Adafruit Bluefruit LE SPI Friend

#include "DashioBluefruitSPI.h"

#define DEVICE_TYPE "TeensyBLEType"
#define DEVICE_NAME "TeensyBLE"

// Create controls. This simple example only requires the controlID for each control
#define TOGGLE_BUTTON_ID "IDB"
#define SINGLE_SLIDER_ID "IDS"
#define SIMPLE_GRAPH_ID "IDG"
#define TEXT_BOX_ID "IDT"
#define DIRECTION_ID "IDDIR"

// Create device
DashioDevice dashioDevice(DEVICE_TYPE);

// Create connection through Bluefruit
DashioBluefruit_BLE  bluefruitBLE(&dashioDevice, true);

// Global variables used throughout
int count = 0;
bool toggle = true;

// Process incoming control mesages
void processStatus() {
    bluefruitBLE.sendMessage(dashioDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
    bluefruitBLE.sendMessage(dashioDevice.getSingleBarMessage(SINGLE_SLIDER_ID, 25));
    bluefruitBLE.sendMessage(dashioDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
    int data[5] = {100, 200, 300, 400, 500};
    bluefruitBLE.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "black", data, 5));
}

void processConfig(MessageData *messageData) {
    Serial.print(F("Screen width in columns: "));
    Serial.println(messageData->idStr);
    
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(DeviceCfg(1, ""))); // one page
    
    ButtonCfg aButton(TOGGLE_BUTTON_ID, "PG01", "Up", {0.05, 0.2, 0.2, 0.3});
    aButton.title = "On/Off";
    aButton.iconName = "up";
    aButton.offColor = "red";
    aButton.onColor = "purple";
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(aButton));
    
    TextBoxCfg aTextBox(TEXT_BOX_ID, "PG01", "Counter", {0.6, 0.0, 0.4, 0.3});
    aTextBox.format = dateTimeFmt;
    aTextBox.kbdType = dateTimeKbd;
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(aTextBox));
    
    SliderCfg aSlider(SINGLE_SLIDER_ID, "PG01", "UV", {0.25, 0.25, 0.3, 0.3});
    aSlider.knobColor = "green";
    aSlider.barColor = "yellow";
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(aSlider));
    
    GraphCfg aGraph(SIMPLE_GRAPH_ID, "PG01", "Level", {0.1, 0.5, 1.0, 0.4});
    aGraph.xAxisLabel = "Temperature";
    aGraph.xAxisMin = 0;
    aGraph.xAxisMax = 1000;
    aGraph.xAxisNumBars = 6;
    aGraph.yAxisLabel = "Mixing Rate";
    aGraph.yAxisMin = 100;
    aGraph.yAxisMax = 600;
    aGraph.yAxisNumBars = 6;
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(aGraph));
    
    DirectionCfg aDir(DIRECTION_ID, "PG01", "Wind", {0.0, 0.0, 0.3, 0.3});
    aDir.units = "Kt";
    aDir.precision = 4;
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(aDir));
    
    DeviceViewCfg deviceViewOne("PG01", "First Page", "down", "dark gray");
    deviceViewOne.ctrlBkgndColor = "blue";
    deviceViewOne.numColumns = 2;
    deviceViewOne.ctrlTitleBoxColor = 5;
    deviceViewOne.shareColumn = false;
    bluefruitBLE.sendMessage(dashioDevice.getConfigMessage(deviceViewOne));
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == TOGGLE_BUTTON_ID) {
        toggle = !toggle;
        
        if (toggle) {
            int data[7] = {50, 255, 505, 758, 903, 400, 0};
            bluefruitBLE.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "Line Name", bar, "0", data, 7));
          int data2[6] = {153, 351, 806, 900, 200, 0};
            bluefruitBLE.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "Line Name", segBar, "1", data2, 6));
        } else {
            float data[7] = {200, 303.3345667, 504.4332, 809.43424545465, 912, 706, 643};
            bluefruitBLE.sendMessage(dashioDevice.getGraphLineFloats(SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "4", data, 7));
            int data2[0] = {};
            bluefruitBLE.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "Line Name", peakBar, "1", data2, 0));
        }
        
        bluefruitBLE.sendMessage(dashioDevice.getButtonMessage(messageData->idStr, toggle, "pad"));
        bluefruitBLE.sendMessage(dashioDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
        int data[2] = {25, 75};
        bluefruitBLE.sendMessage(dashioDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
        
        count += 1;
        bluefruitBLE.sendMessage(dashioDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
    }
}

void processSlider(MessageData *messageData) {
    int data[2];
    data[0] = 100 - messageData->payloadStr.toInt();
    data[1] = messageData->payloadStr.toInt()/2;
    bluefruitBLE.sendMessage(dashioDevice.getDoubleBarMessage(messageData->idStr, data[0], data[1]));
}

void onIncomingMessageCallback(MessageData *messageData) {
    switch (messageData->control) {
        case status:
            processStatus();
            break;
        case config:
            processConfig(messageData);
            break;
        case button:
            processButton(messageData);
            break;
        case slider:
            processSlider(messageData);
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);

    dashioDevice.name = DEVICE_NAME;
    bluefruitBLE.setCallback(&onIncomingMessageCallback);
    bluefruitBLE.begin(true);
}

void loop() {
    bluefruitBLE.checkForMessage();
}
