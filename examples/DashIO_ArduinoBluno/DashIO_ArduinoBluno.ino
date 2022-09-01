// DF Bluno V2.0

#include "DashioBluno.h"
#include <SimpleTimer.h>

#define DEVICE_NAME "Bluno Test"
#define DEVICE_TYPE "Bluno_Type"

const char configC64Str[] PROGMEM =
"3VVLb+IwEP4rlc9oFSi0Wm6kUFQVkhXQrlarPZhkAKuOjRyHx1b89x07NgRIq55XPsQez8PzfTOTd5KBKHLS/f2nQTidA3d7DTsd"
"yh2Y4ztJpNBK8qc+6ZKn/ow0yJoqENoK+psmCgrBNCoT3GqmOeB2Zr8NsgK2XOkJ1UySbvCt2TLLqf2QOUO5QPUwns3iMSlj9zhb"
"GuHDIJoNJihcSJVRjZIojgZ43nvLo9/b72bh0xQkLLc+gwZ5m6ez/RpOhluW6tXRqN1pkN2Vq3aDJFzm8DxPYzEFkZKuVgUcEJiM"
"rh1E80JrKYZKFueSEjK5WDxILhUGHioAgZHPUQyJNxgIOufgYjSIFN7wF3Aut3VofwZpBfkPIf4YPQN+SeM5UK06nJqIHsO0IpqZ"
"sD2t8ZmocNMkBquloutVCce+t2P5mJWc7MxhZKttqvf2xXF0BdCQOE1vVvqgO9JtBkFQg8ruQuESjlPcSqHWwXMBb7tlVh1qnTau"
"jntaxbM9R0UWUoX5d1zoquAc3ft6dA2IWBoPUghIyke+EwU0fXkp034MTd45qA1L4CQMDH2KaajqHbAZhJyXfOQO9yiejHujK+yf"
"rVeB1c/3sZgAB5qj+oLyHEwPlBDXUKAgfaW8QN37GgIucL0NcH1CQ76SWyTfMupCX1MQ3JlV5uYbZ0yVlKblUkb5ozRdlD/j/dGN"
"kXvlEcvgi3PB1Htmq9EwY5y4zk/QV6HcgXKqMj9H0fnQt4GxAcMUErpgS0OmKLK+Fb0y2KJO8+B1nMBMX604YvCIDE3ZX8Sz2QoM"
"OIg95lBkwk8OdFYKjKNzSkt2jKfwbSlSn3tEN/ub0PDlLqVKQcVHjyeZt/i5wrJy2lcyd3YOPfvVwDNFRW6rJtnbpjY3tjzwb3NF"
"X/XyY8sKMHdn8yiSAuwgyrGAEy09QTlnmJFrBLu/GMHnzTD9T5phTpXrhalN2meL8qs/zheG/13HmvoJPh2YgV3tDlXOLN8VRcrk"
"K8uLY9PABuEayaU9Hv4B";


// Create device
DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);

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

void processButton(MessageData *messageData) {
    if (messageData->idStr == TEXT_BOX_ID) {
        count = 0;
        bluno.sendMessage(dashioDevice.getSliderMessage(SINGLE_SLIDER_ID, 10));
        bluno.sendMessage(dashioDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, 90, 10));
        int data[5] = {800, 200, 100, 0, 100};
        bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "bob", line, "red", data, 5));
    }

    if (messageData->idStr == TOGGLE_BUTTON_ID) {
        toggle = !toggle;
        if (toggle) {
            int data[7] = {50, 255, 505, 758, 903, 400, 0};
            bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "Alan", bar, "12", data, 7));
            int data2[6] = {153, 351, 806, 900, 200, 0};
            bluno.sendMessage(dashioDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "Steve", segBar, "15", data2, 6));
        } else {
            float data[7] = {90, 303.3345667, 504.4332, 809.4342, 912, 706, 64};
            bluno.sendMessage(dashioDevice.getGraphLineFloats(SIMPLE_GRAPH_ID, "L1", "Bob", peakBar, "18", data, 7));
        }

        bluno.sendMessage(dashioDevice.getButtonMessage(messageData->idStr, toggle));
        bluno.sendMessage(dashioDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
        int data[2] = {25, 75};
        bluno.sendMessage(dashioDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
    }
}

void processKnob(MessageData *messageData) {
    if (messageData->idStr == KNOB_ID) {
        int data;
        data = 100 - messageData->payloadStr.toInt();
        bluno.sendMessage(dashioDevice.getKnobDialMessage(messageData->idStr, data));
    }
}

void processText(MessageData *messageData) {
    if (messageData->idStr = TEXT_BOX_ID) {
        bluno.sendMessage(dashioDevice.getTextBoxMessage(messageData->idStr, messageData->payloadStr));
    }
}

void processMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus();
        break;
    case button:
        processButton(messageData);
        break;
    case knob:
        processKnob(messageData);
        break;
    case textBox:
        processText(messageData);
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
