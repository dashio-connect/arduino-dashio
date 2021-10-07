// DF Bluno V2.0

#include "DashioBluno.h"
#include <SimpleTimer.h>

#define DEVICE_NAME "Bluno Test"
#define DEVICE_TYPE "Bluno_Type"

// Create device
DashioDevice dashioDevice(DEVICE_TYPE);

// Create connection through Bluefruit
DashioBluno  bluno(&dashioDevice);

// Create controls. This simple example only requires the controlID for each control
#define TOGGLE_BUTTON_ID "IDB"
#define SINGLE_SLIDER_ID "IDS"
#define SIMPLE_GRAPH_ID "IDG"
#define TEXT_BOX_ID "IDT"
#define KNOB_ID "IDK"

// Global variables used throughout
SimpleTimer timer;
int count = 0;
bool toggle = true;

// Timer Interrupt
static void Timer() {
    count ++;
    bluno.sendMessage(dashioDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
} 

// Process incoming contr ol mesages
void processStatus() {
    bluno.sendMessage(dashioDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
    bluno.sendMessage(dashioDevice.getKnobMessage(KNOB_ID, 30));
    bluno.sendMessage(dashioDevice.getKnobDialMessage(KNOB_ID, 50));
}

void processConfig() {
    // Not enough resources for full config, so just do a basic config
    String configData = dashioDevice.getBasicConfigData(button, TOGGLE_BUTTON_ID, "Toggle");
    configData += dashioDevice.getBasicConfigData(slider, SINGLE_SLIDER_ID, "Slider");
    configData += dashioDevice.getBasicConfigData(graph, SIMPLE_GRAPH_ID, "Graph");
    configData += dashioDevice.getBasicConfigData(textBox, TEXT_BOX_ID, "Text");
    configData += dashioDevice.getBasicConfigData(knob, KNOB_ID, "Knob");
    bluno.sendMessage(dashioDevice.getBasicConfigMessage(configData));
}

void processButton(DashioConnection *connection) {
    if (connection->idStr == TEXT_BOX_ID) {
        count = 0;
        bluno.sendMessage(dashioDevice.getSliderMessage(SINGLE_SLIDER_ID, 10));
        bluno.sendMessage(dashioDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, 90, 10));
        int data[5] = {800, 200, 100, 0, 100};
        bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "bob", line, "red", data, 5));
    }

    if (connection->idStr == TOGGLE_BUTTON_ID) {
        toggle = !toggle;
        if (toggle) {
            int data[7] = {50, 255, 505, 758, 903, 400, 0};
            bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "alan", bar, "12", data, 7));
            int data2[6] = {153, 351, 806, 900, 200, 0};
            bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "steve", segBar, "15", data2, 6));
        } else {
            float data[7] = {90, 303.3345667, 504.4332, 809.4342, 912, 706, 64};
            bluno.sendMessage(dashioDevice.getGraphLineFloats(SIMPLE_GRAPH_ID, "L1", "fish", peakBar, "18", data, 7));
        }

        bluno.sendMessage(dashioDevice.getButtonMessage(connection->idStr, toggle));
        bluno.sendMessage(dashioDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
        int data[2] = {25, 75};
        bluno.sendMessage(dashioDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
    }
}

void processKnob(DashioConnection *connection) {
    if (connection->idStr == KNOB_ID) {
        int data;
        data = 100 - connection->payloadStr.toInt();
        bluno.sendMessage(dashioDevice.getKnobDialMessage(connection->idStr, data));
    }
}

void processText(DashioConnection *connection) {
    if (connection->idStr = TEXT_BOX_ID) {
        bluno.sendMessage(dashioDevice.getTextBoxMessage(connection->idStr, connection->payloadStr));
    }
}

void processMessage(DashioConnection *connection) {
    switch (connection->control) {
    case status:
        processStatus();
        break;
    case config:
        processConfig();
        break;
    case button:
        processButton(connection);
        break;
    case knob:
        processKnob(connection);
        break;
    case textBox:
        processText(connection);
        break;
    }
}

void setup() {
    dashioDevice.name = DEVICE_NAME;
    
    bluno.setCallback(&processMessage);
    bluno.begin();
    timer.setInterval(5000, Timer);
}

void loop()
{
    timer.run();
    bluno.checkForMessage();
}
