// Teensy 3.6 with Adafruit Bluefruit LE SPI Friend

#include "DashioBluefruitSPI.h"

#define DEVICE_TYPE "TeensyBLEType"
#define DEVICE_NAME "TeensyBLE"

// Create controls. This simple example only requires the controlID for each control
#define TOGGLE_BUTTON_ID "IDB"
#define SINGLE_SLIDER_ID "IDS"
#define SIMPLE_GRAPH_ID "IDG"
#define TEXT_BOX_ID "IDT"

const char configC64Str[] PROGMEM =
"nVVLb/I4FP0ryIvZNBqR9MFXduQBrRoSmqTpV41mEYgBTx2bOk6btOp/n+s8aApUI428wLHPvbbPPefygVzTReO//tbQneebzSx0"
"7QBmH6goSIrGyDIqfvby5N2PcPk4L//5HU2H3gZpKKckxcJhyZJiAEpRYA2tOJOC01sbIm/tUMEwS31GK58FmOIkx2i8TmgO2Cwp"
"0VgfDjW0SwRmsg6y46EOUQKncUILAI9gXxJJYYqi+ldDW0w2WxkkknA0Hv55YajRwhY8J7DOAO75nqNusOVvc8Lm6rj26KpD7XNc"
"XsC41NAz40uLUy4gPoB3aWiZiCmnlL/lYf3ifRLY6JDhThC2GcwExgxC3kgqt/vUhjEyRucaKo8OHV6rUWcKZVU/MXRmkCAjcP/h"
"J5Qjcn5H/XLcxGGY7i78+7PiNX4nlTMkxcM9OmQ+QqdYLRiROXyi/6RUN9T4kVKJSzmhZKOWLMeLnAAW11xkifwC/UzyTuAVyeuM"
"UN3nZRpVO/wV+J2+LuiYvvMrNeDplOf4bgkyC0FsjRQVd/Zt0Gh6Plm0E8d7aGauP2smk9juhG9FLd5y24kdx481+3lbHtMPbCdQ"
"ul5JQUFSU6A9JO+wdw5P2QiSgiaKjAHNhqG0B0VoVjqLsCLbQ/TvdWvLpFKbXIDWOn09bonE3c7zhqXdhqk80qz/iI1EwvJaDKuq"
"Jny1D05Wzz0t2CShgz8Gd2CBNr5Wh8nLE8d1W0fZFQcBuAUYOe9Bv4jSf2mIwLO9JFOnPuxQU6xJ24usm+Cb5M+m+sILw0VYvJw9"
"VemTPVmbLxMlsElJ8jlpVFSqDzdZYpp3XvK9I18oc5X9sCZH24lOtqLyAHHonK+De846tsyBwS51GMYJk7RX6iWsv70iMxMBpF62"
"J/YX+obRTzilbiSzYHHTEGxGkdcneKRfzH6R7Cm48YP4fW1Xi+uXXPUUvl53lbeqhB2xaar2WEjJ2cHfAGddnEsyfLIXHfCheqQx"
"Omb3BzaPe8vV9dX1qOlMAFsInOeDOf4f3aQnTVddcWAWdFlL1IxmbR+xptA9PlCKX8kKh1gWO0DP76NIi6yFZrrqiuBzu96PCX5r"
"vb7eBPgVpp+f/wI=";

// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);

// Create connection through Bluefruit
DashBluefruit_BLE  bluefruitBLE(&dashDevice, true);

// Global variables used throughout
int count = 0;
bool toggle = true;

// Process incoming control mesages
void processStatus() {
    bluefruitBLE.sendMessage(dashDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
    bluefruitBLE.sendMessage(dashDevice.getSingleBarMessage(SINGLE_SLIDER_ID, 25));
    bluefruitBLE.sendMessage(dashDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
    int data[5] = {100, 200, 300, 400, 500};
    String message = "";
    dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "black", yLeft, data, 5);
    bluefruitBLE.sendMessage(message);
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == TOGGLE_BUTTON_ID) {
        toggle = !toggle;
        
        String message = "";
        if (toggle) {
            int data[7] = {50, 255, 505, 758, 903, 400, 0};
            dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L1", "Line Name", bar, "0", yLeft, data, 7);
            bluefruitBLE.sendMessage(message);
            int data2[6] = {153, 351, 806, 900, 200, 0};
            dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L2", "Line Name", segBar, "1", yLeft, data2, 6);
            bluefruitBLE.sendMessage(message);
        } else {
            float data[7] = {200, 303.3345667, 504.4332, 809.43424545465, 912, 706, 643};
            dashDevice.addChartLineFloats(message, SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "4", yLeft, data, 7);
            bluefruitBLE.sendMessage(message);
            int data2[0] = {};
            dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L2", "Line Name", peakBar, "1", yLeft, data2, 0);
            bluefruitBLE.sendMessage(message);
        }
        
        bluefruitBLE.sendMessage(dashDevice.getButtonMessage(messageData->idStr, toggle, "weather"));
        bluefruitBLE.sendMessage(dashDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
        int data[2] = {25, 75};
        bluefruitBLE.sendMessage(dashDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
        
        count += 1;
        bluefruitBLE.sendMessage(dashDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
    }
}

void processSlider(MessageData *messageData) {
    int data[2];
    data[0] = 100 - messageData->payloadStr.toInt();
    data[1] = messageData->payloadStr.toInt()/2;
    bluefruitBLE.sendMessage(dashDevice.getDoubleBarMessage(messageData->idStr, data[0], data[1]));
}

void onIncomingMessageCallback(MessageData *messageData) {
    switch (messageData->control) {
        case status:
            processStatus();
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

    dashDevice.name = DEVICE_NAME;
    bluefruitBLE.setCallback(&onIncomingMessageCallback);
    bluefruitBLE.begin(true);
}

void loop() {
    bluefruitBLE.run();
}
