// DF Bluno V2.0

#include "DashIO.h"
#include <SimpleTimer.h>

#define DEVICE_NAME "Bluno Test"
#define DEVICE_TYPE "bluno"

// Create device
DashDevice myDevice;

// Create Connection
DashConnection myBLEconnection(BLE_CONN);

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
  bleWriteStr(myDevice.getTextBoxMessage(TEXT_BOX_ID, String(count)));
} 

static void bleWriteStr(String message) {
  Serial.print(message);
}

// Process incoming contr ol mesages
void processStatus() {
  bleWriteStr(myDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
  bleWriteStr(myDevice.getKnobMessage(KNOB_ID, 30));
  bleWriteStr(myDevice.getKnobDialMessage(KNOB_ID, 50));
}

void processConfig() {
  // Not enough code space for full config, so just do a basic config
  String configData = myDevice.getBasicConfigData(button, TOGGLE_BUTTON_ID, "Toggle");
  configData += myDevice.getBasicConfigData(slider, SINGLE_SLIDER_ID, "Slider");
  configData += myDevice.getBasicConfigData(graph, SIMPLE_GRAPH_ID, "Graph");
  configData += myDevice.getBasicConfigData(textBox, TEXT_BOX_ID, "Text");
  configData += myDevice.getBasicConfigData(knob, KNOB_ID, "Knob");
  bleWriteStr(myDevice.getBasicConfigMessage(configData));
}

void processButton() {
  if (myBLEconnection.idStr == TEXT_BOX_ID) {
    count = 0;
    bleWriteStr(myDevice.getSliderMessage(SINGLE_SLIDER_ID, 10));
    bleWriteStr(myDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, 90, 10));
    int data[5] = {800, 200, 100, 0, 100};
    bleWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "bob", line, "red", data, 5));
  }

  if (myBLEconnection.idStr == TOGGLE_BUTTON_ID) {
    toggle = !toggle;
    if (toggle) {
      int data[7] = {50, 255, 505, 758, 903, 400, 0};
      bleWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "alan", bar, "12", data, 7));
      int data2[6] = {153, 351, 806, 900, 200, 0};
      bleWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "steve", segBar, "15", data2, 6));
    } else {
      float data[7] = {90, 303.3345667, 504.4332, 809.4342, 912, 706, 64};
      bleWriteStr(myDevice.getGraphLineFloats(SIMPLE_GRAPH_ID, "L1", "fish", peakBar, "18", data, 7));
    }

    bleWriteStr(myDevice.getButtonMessage(myBLEconnection.idStr, toggle));
    bleWriteStr(myDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
    int data[2] = {25, 75};
    bleWriteStr(myDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
  }
}

void processKnob() {
  if (myBLEconnection.idStr == KNOB_ID) {
    int data;
    data = 100 - myBLEconnection.payloadStr.toInt();
    bleWriteStr(myDevice.getKnobDialMessage(myBLEconnection.idStr, data));
  }
}

void processText() {
  if (myBLEconnection.idStr = "TB") {
    bleWriteStr(myDevice.getTextBoxMessage(myBLEconnection.idStr, myBLEconnection.payloadStr));
  }
}

void processMessage() {
  switch (myBLEconnection.control) {
    case who:
      bleWriteStr(myDevice.getWhoMessage(DEVICE_NAME, DEVICE_TYPE));
      break;
    case connect:
      bleWriteStr(myDevice.getConnectMessage());
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
    case knob:
      processKnob();
      break;
    case textBox:
      processText();
      break;
  }
}

void checkIncomingMessages() {
  while(Serial.available()) {
    char data;
    data = (char)Serial.read();

    if (myBLEconnection.processChar(data)) {
      processMessage();
    }
  }
}

void setupBLE() {
  Serial.begin(115200); //initialise the Serial

// Set peripheral name
  delay(350);
  Serial.print("+++"); // Enter AT mode
  delay(500);
  Serial.println(F("AT+NAME=" DEVICE_TYPE)); // If the name has changed, requires a RESTART or power cycle
  delay(500);
  Serial.println("AT+NAME=?");
  delay(500);

  // Get deviceID for mac address.
  while(Serial.available() > 0) {Serial.read();} // But first, clear the serial.
  Serial.println("AT+MAC=?");
  delay(1000); // Wait a while for reply
  myDevice.deviceID = "";
  while(Serial.available() > 0) {
    char c = Serial.read();
    if ((c == '\n') || (c == '\r')) { // Before the OK
      break;
    }
    myDevice.deviceID += c;
  }

  Serial.println("AT+EXIT");
  delay(1000);
  bleWriteStr(myDevice.getWhoMessage(DEVICE_NAME, DEVICE_TYPE)); // In case WHO message is received when in AT mode

  timer.setInterval(5000, Timer);
}

void setup() {
  setupBLE();
}

void loop()
{
  timer.run();
  checkIncomingMessages();
}
