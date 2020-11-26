// Arduino Ethernet shield attached to pins 10, 11, 12, 13
 
#include <Ethernet.h>
#include <timer.h>
#include "DashIO.h"

#define DEVICE_TYPE "TCP_Stuff"
#define DEVICE_NAME "TCP Example"

#define TCP_PORT 5000
byte mac[] = {0xFE, 0xED, 0xCA, 0xFE, 0xBA, 0xBE}; // mac must be unique on your network segment

// Create device
const DashDevice myDevice;

// Create Connection
const DashConnection myTCPconnection(TCP_CONN);

// Create controls. This simple example only requires the controlID for each control
const String SINGLE_SLIDER_ID = "IDS";
const String SIMPLE_GRAPH_ID = "IDG";
const String TEXT_ID = "IDT";
const String TOGGLE_BUTTON_ID = "IDB";
bool toggle = true; // Used for the button control

// Global variables used throughout
EthernetServer server(TCP_PORT);
auto timer = timer_create_default();

int count = 0;
boolean alreadyConnected = false; // whether or not the client was connected previously

// Timer Callback
bool onTimerCallback() {
  // onTimerCallback is not an interrupt, so it is safe to read data and send messages from within.
  count ++;
  tcpWriteStr(myDevice.getTextBoxMessage(TEXT_ID, String(count)));

  return true; // true to repeat the timer action
}

void tcpWriteStr(String message) {
  if (alreadyConnected) {
    Serial.println("**** Try TCP Send ****");
    Serial.println(message);
    server.print(message);
  }
}

// Process incoming control mesages
void processStatus() {
  tcpWriteStr(myDevice.getDeviceNameMessage(DEVICE_NAME));
  tcpWriteStr(myDevice.getButtonMessage(TOGGLE_BUTTON_ID, toggle));
  tcpWriteStr(myDevice.getSingleBarMessage(SINGLE_SLIDER_ID, 25));
  int data[5] = {100, 200, 300, 400, 500};
  tcpWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "black", data, 5));
}

void processConfig() {
  // Not enough code space for full config, so just do a basic config
  String configData = myDevice.getBasicConfigData(button, TOGGLE_BUTTON_ID, "Toggle");
  configData += myDevice.getBasicConfigData(slider, SINGLE_SLIDER_ID, "Slider");
  configData += myDevice.getBasicConfigData(graph, SIMPLE_GRAPH_ID, "Graph");
  configData += myDevice.getBasicConfigData(textBox, TEXT_ID, "Text");
  tcpWriteStr(myDevice.getBasicConfigMessage(configData));
}

void processButton() {
  if (myTCPconnection.idStr == TOGGLE_BUTTON_ID) {
    toggle = !toggle;
    if (toggle) {
      int data[7] = {50, 255, 505, 758, 903, 400, 0};
      tcpWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L1", "Line Name", bar, "0", data, 7));
      int data2[6] = {153, 351, 806, 900, 200, 0};
      tcpWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "Line Name", segBar, "1", data2, 6));
    } else {
      float data[7] = {200, 303.3345667, 504.4332, 809.43424545465, 912, 706, 643};
      tcpWriteStr(myDevice.getGraphLineFloats(SIMPLE_GRAPH_ID, "L1", "Line Name", peakBar, "4", data, 7));
      int data2[0] = {};
      tcpWriteStr(myDevice.getGraphLineInts(SIMPLE_GRAPH_ID, "L2", "Line Name", peakBar, "1", data2, 0));
    }

    tcpWriteStr(myDevice.getButtonMessage(myTCPconnection.idStr, toggle));
    tcpWriteStr(myDevice.getSliderMessage(SINGLE_SLIDER_ID, 75));
    int data[2] = {25, 75};
    tcpWriteStr(myDevice.getDoubleBarMessage(SINGLE_SLIDER_ID, data[0], data[1]));
  }
}

void processSlider() {
  int data[2];
  data[0] = 100 - myTCPconnection.payloadStr.toInt();
  data[1] = myTCPconnection.payloadStr.toInt()/2;
  tcpWriteStr(myDevice.getDoubleBarMessage(myTCPconnection.idStr, data[0], data[1]));
}

void processIncomingMessage() {
  switch (myTCPconnection.control) {
    case who:
      tcpWriteStr(myDevice.getWhoMessage(DEVICE_NAME, DEVICE_TYPE));
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
    case slider:
      processSlider();
      break;
  }
}

void checkIncomingMessages() {
  // wait for a new client:
  EthernetClient client = server.available();
  client.setTimeout(2000);

  if (client) {
    if (!alreadyConnected) {
      client.flush();  // clear out the input buffer:
      Serial.println("Connected");
      alreadyConnected = true;
    }
    
    while(client.available()) {
      char data;
      data = (char)client.read();
  
      if (myTCPconnection.processChar(data)) {
        Serial.println("**** TCP Received ****");
        Serial.print(myTCPconnection.deviceID);
        Serial.print("   " + myDevice.getControlTypeStr(myTCPconnection.control));
        Serial.println("   " + myTCPconnection.idStr + "   " + myTCPconnection.payloadStr);

        processIncomingMessage();
      }
    }
  }
}

void setupTCP() {
  myDevice.setDeviceID(mac);

  // initialize the ethernet device
  Ethernet.begin(mac); // start listening for clients
  server.begin(); // Open serial communications and wait for port to open:

  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());

  timer.every(1000, onTimerCallback); // 1000ms
}

void setup() {
  Serial.begin(115200);

  setupTCP();
}

void loop() {
  timer.tick();
  checkIncomingMessages();
}
