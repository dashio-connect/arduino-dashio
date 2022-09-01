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
"hVTbctowEP2VjJ6hNZfQwhsXh8mUAAMOmU7bB4EXWxNZYmQ5QDP8e1eyBThAOnqRjvamoz37ThIQWUo6v/5UCKdL4MVew0735A7M"
"8Z2spNBK8scB6ZDHQUAqZEMVCG2B6dCrIZIJptGa4FYzzQG3fZkJDQqRGFgU6xnVTJKO96V2b1ZhOJUpQ1ygQzCZkjx3l7PIIH1/"
"HPgzBNdSJVQjMugGfvD45CO2d67HwK0GriaWp2DFUhvUq5DXZRjsN1B23rJQxydHrGZ3Ea5RISsuU/ixDCdiDiIkHa0yOCBBCd0U"
"VC0zraUYKpmVkZw6uV73JZcKk88gxLxlLnvEmfuCLjkUGSpECuc2YglcZfwzUtFqIn5/nazX5BbNt9kzH5D/ZJmk+jWOPEzJ8FFj"
"mpisL0B1jH9uOIoU3cQ5DfvujqVPDLPXPPyRnTmObL/N9T4vd3xBzpAUltYR3fIodEc6LRPlkpLd0QDTeCcuRvAGnJznNTRAsgFF"
"dabgJkkfOG60zbrCXVGbC/3EdkxEd3hrQturcZb0qEI2WkUZ50CZ5/Z1ng2l2CJ9KQSs8irfiQIaPj9bDlp+Ex/tNaq9+26j+oCV"
"Vn2v2676955fbw76/W7bNH4K6o2toOxU+4/TVjH9waX+qcsBZSfkslBEyKibLCts60wVB8qpStzIwT4fuo4xPmDKxNeuWWReKrJk"
"YKEFgy3a1A7OpgDMoNKK4/c/YBvN2V/8+Qb+TBpjn6CasgQZW2MhKC8MliPoVy/3XdFKJlTvNRKh02GPZ+BwqUJQE+HEesKc8UvM"
"tLO+wIrzNFMbfmy9XHDHrIGiIrUNvtrb/jI3gTHEsewiTtAogrtisJwZ3PY+UVP7XhLuQG6FVW0KHNtLui9KOcN35fTm+w+TqqzZ"
"ue0wgfOS7ydihrFoCs40KZR5TboKwgU1FHe+nQn3eXFl0H0365Zi01hucV7YIZBnvRx0zbpZeYc6LocKQJhpTNWD5Fxu07l9rYuC"
"uDP9Ceb+YjzekK11dVNu7puhlth5drDCULmWnSCykMkFS7OjXnByCT2SkT0e/gE=";

// Create device
DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);

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
        
        bluefruitBLE.sendMessage(dashioDevice.getButtonMessage(messageData->idStr, toggle, "weather"));
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
