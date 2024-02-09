// DF Bluno V2.0

#include "DashioBluno.h"
#include <SimpleTimer.h>

#define DEVICE_NAME "Bluno Test"
#define DEVICE_TYPE "Bluno_Type"

const char configC64Str[] PROGMEM =
"vVXbdto4FP2VWXod1hQ7YdryhrFDCL4E24WQtg++CFAwErVlbln59x7JNnXASWdeuvSAfG6S9tln84xMzUTdr99baGQ7GuyeUZ6T"
"GHXRtw9Xms8XNHKtv4f0/sE0lrq56aEWyvghwRBhO67VM8EQMcpTlgx1MA71kQjBNHZocnCoixMcZBA+D5IMt9A62KOu0m630CZI"
"MeUySZ8okJTieBIkOcR+BDcnXJ7iy98WWmKyWHI34IShbvufqzasMuqeZQTMFKI1x/cdS9xgyXYWoZY4rjz6UMWdinQUWGoLrSgL"
"+yxhKVSwgpRBqRaKSZDcsCRhu2wE/lMZYa+CdRYvcPqXJm7dQjsS8+Wp+EcVFlxxf3GsKl2ABYErt18AfM/U3Tr4d+oj7TwMV2P6"
"Y7tdkNEWj11/J56VkBinBg3CBEMgT3N8jr/3R/C/7oj1HxtQXPN/4h8GaQm/Jx99egI4quAZFgEX0KtvQS+TvZK/njFA9Sb4xoNf"
"b8LtcXoMjwPia87yMbx1ZsfpOB9f8N1HDVjmlPAMvtBvgVRUsd4BkuM97yVkIYx9w/YNF4xzlq4DLofQNlATtp+uYUGDNimOSCZr"
"wvNXYewfNvhX4mvgOk10BWOUsAyPQuCUB8wqGiog04duoR5W777cGPaXYmc6g2LTm+jFxjP7fhnfN8uNPplMJeiVqmiOqxuuIHHE"
"0wTocwNoe+QIPkWFFyxSEkP78zUFfFVVEA3ALywV02i+PoUor/tVtEeU1lgKrKqYNEjxoXKsFjSu7HawPVTzLZyVfbokHNcT/DSg"
"meRBdJBQR2WklgTRqoEHIlPuNba/IH/deVFZQODCXKDu1VUttAbUvy1E4NV2sJatZhSjol29Uu37t+4rrkcKHo9XPx79gTe/i2bG"
"U/vOCgXXD709ySxSsGcvPswgxElWDZFjXwyEmKp9Pa2oUQpPk/LszwLOofp1bm2kmmblXKTkEDRMR3mnWkn5bedrLUgB1055Zt1Q"
"nxSlYU6khAzc+9sCYc337TrCT0+frW8f1Hnm9oINu57OzJU/2wntYvN51X+TrPEFnpoQw5xzRs90n9EqzwVjA67vSU0N3jfhfFtV"
"hCgVwL0WEOWTWO/96dV46RMYjBeJ1aCUj/4NiMYzivGWRNjDPN+I+5hCqWCodWmeELwrB3u+cPEWti8vPwE=";


// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);

// Create connection through Bluefruit
DashBluno  bluno(&dashDevice);

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
String messageToSend = ((char *)0);

// Timer Interrupt
static void Timer() {
    count ++;
    messageToSend = dashDevice.getTextBoxMessage(TEXT_BOX_ID, String(count));
} 

// Process incoming contr ol mesages
void processStatus() {
    bluno.sendMessage(dashDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
    bluno.sendMessage(dashDevice.getKnobMessage(KNOB_ID, 30));
    bluno.sendMessage(dashDevice.getKnobDialMessage(KNOB_ID, 50));
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == TEXT_BOX_ID) {
        count = 0;
        bluno.sendMessage(dashDevice.getSliderMessage(SINGLE_SLIDER_ID, 10));
        bluno.sendMessage(dashDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, 90, 10));
        int data[5] = {800, 200, 100, 0, 100};
        String message = "";
        dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L1", "bob", line, "red", yLeft, data, 5);
        bluno.sendMessage(message);
    }

    if (messageData->idStr == TOGGLE_BUTTON_ID) {
        toggle = !toggle;
        String message = "";
        if (toggle) {
            int data[7] = {50, 255, 505, 758, 903, 400, 0};
            dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L1", "Alan", bar, "12", yLeft, data, 7);
            bluno.sendMessage(message);
            int data2[6] = {153, 351, 806, 900, 200, 0};
            dashDevice.addChartLineInts(message, SIMPLE_GRAPH_ID, "L2", "Steve", segBar, "15", yLeft, data2, 6);
            bluno.sendMessage(message);
        } else {
            float data[7] = {90, 303.3345667, 504.4332, 809.4342, 912, 706, 64};
            dashDevice.addChartLineFloats(message, SIMPLE_GRAPH_ID, "L1", "Bob", peakBar, "18", yLeft, data, 7);
            bluno.sendMessage(message);
        }

        bluno.sendMessage(dashDevice.getButtonMessage(messageData->idStr, toggle));
        bluno.sendMessage(dashDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
        int data[2] = {25, 75};
        bluno.sendMessage(dashDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
    }
}

void processKnob(MessageData *messageData) {
    if (messageData->idStr == KNOB_ID) {
        int data;
        data = 100 - messageData->payloadStr.toInt();
        bluno.sendMessage(dashDevice.getKnobDialMessage(messageData->idStr, data));
    }
}

void processText(MessageData *messageData) {
    if (messageData->idStr = TEXT_BOX_ID) {
        bluno.sendMessage(dashDevice.getTextBoxMessage(messageData->idStr, messageData->payloadStr));
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
    dashDevice.name = DEVICE_NAME;
    
    bluno.setCallback(&processMessage);
    bluno.begin();
    timer.setInterval(5000, Timer);
}

void loop()
{
    timer.run();
    bluno.run();
    if (messageToSend.length() > 0) {
        bluno.sendMessage(messageToSend);
        messageToSend = "";
    }
}
