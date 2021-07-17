#include <BLEDevice.h> // ESP32 BLE Arduino library by Neil Kolban. Included in Arduino IDE
#include "DashIO.h"
#include <EEPROM.h>

#define DEVICE_TYPE "SPARK32"
#define DEVICE_NAME "Clamshell"

// Bluetooth Light (BLE)
// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define EEPROM_SIZE 244

struct EEPROMSetupObject {
    char deviceName[32];
    char saved;
};

EEPROMSetupObject userSetup = {DEVICE_NAME, 'Y'};

// Create BLE client characteristic
BLECharacteristic *pCharacteristic;

// Create device
DashDevice myDevice;
  
// Create Connections
// Note: if you change the BLE connectionn name, you must cycle the power of the ESP32 before the new name change will take effect.
DashConnection myBLEconnection(BLE_CONN);

// Create controls
int menuSelectorIndex = 0;
float menuTextBoxValue = 12.34;
bool menuButtonValue = on;
bool buttonOneValue = on;
bool buttonTwoValue = off;
bool buttonThreeValue = on;
bool buttonFourValue = off;
bool buttonFiveValue = on;
float menuSliderValue = 14;

// Create Control IDs
// Device Views
const char *DV01_ID = "DV01";
const char *DV02_ID = "DV02";

// Device View 1 Controls
const char *LOG_ID = "EL01";
const char *BUTTON02_ID = "UB02";

const char *MENU_ID = "M01";
const char *MENU_SELECTOR_ID = "SLR01";
const char *MENU_BUTTON_ID = "UB01";
const char *MENU_TEXTBOX_ID = "CTB01";
const char *MENU_SLIDER_ID = "SLD01";

const char *BGROUP_ID = "BG01";
const char *BGROUP_B01_ID = "GB01";
const char *BGROUP_B02_ID = "GB02";
const char *BGROUP_B03_ID = "GB03";
const char *BGROUP_B04_ID = "GB04";
const char *BGROUP_B05_ID = "GB05";

// Device View 2 Controls
const char *GRAPH_ID = "IDG";
const char *LABEL_ID = "LBL01";
const char *KNOB_ID = "KB01";
const char *DIAL_ID = "DL01";

// Hardware timer
hw_timer_t * timer = NULL;

String messageToSend = "";

class securityBLECallbacks : public BLESecurityCallbacks {
  bool onConfirmPIN(uint32_t pin){
    Serial.print(F("Confirm Pin: "));
    Serial.println(pin);
    return true;  
  }
  
  uint32_t onPassKeyRequest(){
    Serial.println(F("onPassKeyRequest"));
    return 0;
  }

  void onPassKeyNotify(uint32_t pass_key){
    Serial.println(F("onPassKeyNotify"));
  }

  bool onSecurityRequest(){
    Serial.println(F("onSecurityRequest"));
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    if(cmpl.success){
      Serial.println(F("Authentication Success"));
    } else{
      Serial.println(F("Authentication Fail"));
    }
  }
};

// BLE callback for when a message is received
class messageReceivedBLECallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string payload = pCharacteristic->getValue();
    myBLEconnection.processMessage(payload.c_str()); // The message components are stored within the connection where the messageReceived flag is set
  }
};

void saveUserSetup() {
  EEPROM.put(0, userSetup);
  EEPROM.commit();
}

void loadUserSetup() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
      Serial.println(F("Failed to init EEPROM"));
  } else {
    EEPROMSetupObject userSetupRead;
    EEPROM.get(0, userSetupRead);
    if (userSetupRead.saved != 'Y') {
      saveUserSetup();
    } else {
      userSetup = userSetupRead;
    }

    Serial.print(F("Device Name: "));
    Serial.println(userSetupRead.deviceName);
  }
}

void bleWriteStr(String message) {
  pCharacteristic->setValue(message.c_str());
  pCharacteristic->notify();
    
  Serial.println(F("---- BLE Sent ----"));
  Serial.print(F("Message: "));
  Serial.println(message);
}

// Process incoming control mesages
void processStatus() {
  // Control initial condition messages (recommended)
  String selection[] = {F("Dogs"), F("Bunnys"), F("Pigs")};
  String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex, selection, 3);
  message += myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
  message += myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
  message += myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
  bleWriteStr(message);

  message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
  message += myDevice.getButtonMessage(BGROUP_B02_ID, buttonTwoValue);
  message += myDevice.getButtonMessage(BGROUP_B03_ID, buttonThreeValue);
  message += myDevice.getButtonMessage(BGROUP_B04_ID, buttonFourValue);
  message += myDevice.getButtonMessage(BGROUP_B05_ID, buttonFiveValue);
  bleWriteStr(message);

  int data1[] = {150, 270, 390, 410, 400};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L1", "Line One", line, "3", data1, sizeof(data1)/sizeof(int)));
  int data2[] = {160, 280, 400, 410, 420};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L2", "Line Two", line, "4", data2, sizeof(data2)/sizeof(int)));
  int data3[] = {170, 290, 410, 400, 390};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L3", "Line Three", line, "5", data3, sizeof(data3)/sizeof(int)));
  int data4[] = {180, 270, 390, 410, 430};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L4", "Line Four", line, "6", data4, sizeof(data4)/sizeof(int)));
  int data5[] = {190, 280, 380, 370, 360};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L5", "Line Five", line, "7", data5, sizeof(data5)/sizeof(int)));
  int data6[] = {200, 250, 260, 265, 240};
  bleWriteStr(myDevice.getGraphLineInts(GRAPH_ID, "L6", "Line Six", line, "8", data6, sizeof(data6)/sizeof(int)));
}

void processConfig() {
  bleWriteStr(myDevice.getConfigMessage(DeviceCfg(2))); // Two Device Views

  EventLogCfg anEventLog(LOG_ID, DV01_ID, "Log", {0.05, 0.545, 0.9, 0.212});
  bleWriteStr(myDevice.getConfigMessage(anEventLog));
  
  ButtonCfg aButton(BUTTON02_ID, DV01_ID, "Event", {0.5, 0.788, 0.2, 0.121});
  aButton.iconName = "bell";
  aButton.offColor = "blue";
  aButton.onColor = "black";
  bleWriteStr(myDevice.getConfigMessage(aButton));

  MenuCfg aMenu(MENU_ID, DV01_ID, "Settings", {0.05, 0.788, 0.4, 0.121});
  aMenu.iconName = "pad";
  aMenu.text = "Setup";
  bleWriteStr(myDevice.getConfigMessage(aMenu));

  SelectorCfg menuSelector(MENU_SELECTOR_ID, MENU_ID, "Selector");
  bleWriteStr(myDevice.getConfigMessage(menuSelector));

  ButtonCfg menuButton(MENU_BUTTON_ID,  MENU_ID, "On/Off");
  menuButton.offColor = "red";
  menuButton.onColor = "purple";
  bleWriteStr(myDevice.getConfigMessage(menuButton));

  TextBoxCfg menuTextBox(MENU_TEXTBOX_ID, MENU_ID, "Counter");
  menuTextBox.units = "°C";
  bleWriteStr(myDevice.getConfigMessage(menuTextBox));

  SliderCfg menuSlider(MENU_SLIDER_ID, MENU_ID, "UV");
  menuSlider.knobColor = "green";
  menuSlider.barColor = "yellow";
  bleWriteStr(myDevice.getConfigMessage(menuSlider));

  ButtonGroupCfg aGroup(BGROUP_ID, DV01_ID, "Actions", {0.75, 0.788, 0.2, 0.121});
  aGroup.iconName = "pad";
  aGroup.text = "BG";
  bleWriteStr(myDevice.getConfigMessage(aGroup));

  ButtonCfg groupButton1(BGROUP_B01_ID, BGROUP_ID, "Up");
  groupButton1.iconName = "up";
  groupButton1.offColor = "red";
  groupButton1.onColor = "purple";
  bleWriteStr(myDevice.getConfigMessage(groupButton1));

  ButtonCfg groupButton2(BGROUP_B02_ID, BGROUP_ID, "Down");
  groupButton2.iconName = "down";
  groupButton2.offColor = "Green";
  groupButton2.onColor = "#9d9f2c";
  bleWriteStr(myDevice.getConfigMessage(groupButton2));

  ButtonCfg groupButton3(BGROUP_B03_ID, BGROUP_ID, "Left");
  groupButton3.iconName = "left";
  groupButton3.offColor = "#123456";
  groupButton3.onColor = "#228855";
  bleWriteStr(myDevice.getConfigMessage(groupButton3));

  ButtonCfg groupButton4(BGROUP_B04_ID, BGROUP_ID, "Right");
  groupButton4.iconName = "right";
  groupButton4.offColor = "red";
  groupButton4.onColor = "#F4A37C";
  bleWriteStr(myDevice.getConfigMessage(groupButton4));

  ButtonCfg groupButton5(BGROUP_B05_ID, BGROUP_ID, "Stop");
  groupButton5.iconName = "stop";
  groupButton5.offColor = "Black";
  groupButton5.onColor = "#bc41d7";
  bleWriteStr(myDevice.getConfigMessage(groupButton5));

  GraphCfg aGraph(GRAPH_ID, DV02_ID, "Level", {0.0, 0.0, 1.0, 0.485});
  aGraph.xAxisLabel = "Temperature";
  aGraph.xAxisMin = 0;
  aGraph.xAxisMax = 1000;
  aGraph.xAxisNumBars = 6;
  aGraph.yAxisLabel = "Mixing Rate";
  aGraph.yAxisMin = 100;
  aGraph.yAxisMax = 600;
  aGraph.yAxisNumBars = 6;
  bleWriteStr(myDevice.getConfigMessage(aGraph));

  LabelCfg aLabel(LABEL_ID, DV02_ID, "Label", {0.0, 0.515, 1.0, 0.394});
  aLabel.color = 22;
  bleWriteStr(myDevice.getConfigMessage(aLabel));

  KnobCfg aKnob(KNOB_ID, DV02_ID, "Knob", {0.05, 0.576, 0.4, 0.303});
  aKnob.knobColor = "#FF00F0";
  aKnob.dialColor = "yellow";
  bleWriteStr(myDevice.getConfigMessage(aKnob));

  DialCfg aDial(DIAL_ID, DV02_ID, "Dial", {0.55, 0.576, 0.4, 0.303});
  aDial.dialFillColor = "green";
  aDial.pointerColor = "yellow";
  aDial.style = dialPieInverted;
  aDial.units = "F";
  aDial.precision = 2;
  bleWriteStr(myDevice.getConfigMessage(aDial));

  // Device Views
  DeviceViewCfg deviceViewOne(DV01_ID, "First Device View", "down", "dark gray");
  deviceViewOne.ctrlBkgndColor = "blue";
  deviceViewOne.ctrlTitleBoxColor = 5;
  bleWriteStr(myDevice.getConfigMessage(deviceViewOne));
  
  DeviceViewCfg deviceViewTwo(DV02_ID, "Second Device View", "up", "0");
  deviceViewTwo.ctrlBkgndColor = "blue";
  deviceViewTwo.ctrlTitleBoxColor = 5;
  bleWriteStr(myDevice.getConfigMessage(deviceViewTwo));
}

void processButton() {
  if (myBLEconnection.idStr == MENU_BUTTON_ID) {
    menuButtonValue = !menuButtonValue;
    String message = myDevice.getButtonMessage(MENU_BUTTON_ID, menuButtonValue);
    bleWriteStr(message);
  } else if (myBLEconnection.idStr == BGROUP_B01_ID) {
    buttonOneValue = !buttonOneValue;
    String message = myDevice.getButtonMessage(BGROUP_B01_ID, buttonOneValue);
    bleWriteStr(message);
  } else if (myBLEconnection.idStr == BUTTON02_ID) {
    String textArr[] = {"one", "two"};
    String message = myDevice.getEventLogMessage(LOG_ID, "", "red", textArr, 2);
    bleWriteStr(message);
  }
}

void processIncomingMessage() {
  switch (myBLEconnection.control) {
    case who:
      bleWriteStr(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE));
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
    case textBox:
      if (myBLEconnection.idStr == MENU_TEXTBOX_ID) {
        menuTextBoxValue = myBLEconnection.payloadStr.toFloat();
        String message = myDevice.getTextBoxMessage(MENU_TEXTBOX_ID, String(menuTextBoxValue));
        bleWriteStr(message);
      }
      break;
    case slider:
      if (myBLEconnection.idStr == MENU_SLIDER_ID) {
        menuSliderValue = myBLEconnection.payloadStr.toFloat();
        String message = myDevice.getSliderMessage(MENU_SLIDER_ID, menuSliderValue);
        bleWriteStr(message);
      }
      break;
    case selector:
      if (myBLEconnection.idStr == MENU_SELECTOR_ID) {
        menuSelectorIndex = myBLEconnection.payloadStr.toInt();
        String message = myDevice.getSelectorMessage(MENU_SELECTOR_ID, menuSelectorIndex);
        bleWriteStr(message);
      }
      break;
    case knob:
      if (myBLEconnection.idStr == KNOB_ID) {
        String message = myDevice.getDialMessage(DIAL_ID, myBLEconnection.payloadStr);
        bleWriteStr(message);
      }
      break;
    case deviceName:
      if (myBLEconnection.idStr != "") {
        myBLEconnection.idStr.toCharArray(userSetup.deviceName, myBLEconnection.idStr.length() + 1);
        saveUserSetup();
        Serial.print(F("Updated Device Name: "));
        Serial.println(userSetup.deviceName);
        bleWriteStr(myDevice.getPopupMessage(F("Name Change Success"), "Changed to :" + myBLEconnection.idStr, F("Happyness")));
      } else {
        bleWriteStr(myDevice.getPopupMessage(F("Name Change Fail"), F("Fish"), F("chips")));
      }
      bleWriteStr(myDevice.getDeviceNameMessage(userSetup.deviceName));
      break;
  }
}

void checkIncomingMessages() {
  // Never process messages from within an interrupt.
  // Instead, look to see if the messageReceived flag is set for each connection.

  // Look for BLE messages
  if (myBLEconnection.messageReceived) {
    myBLEconnection.messageReceived = false;
    processIncomingMessage();
  }
}

void checkOutgoingMessages() {
  // Never send messages from within an interrupt.
  // Instead, look to see if there is a meassge ready to send in messageToSend.
  if (messageToSend.length() > 0) {
    bleWriteStr(messageToSend);
    messageToSend = "";
  }
}

void setupBLE() {
  esp_bt_controller_enable(ESP_BT_MODE_BLE); // Make sure we're only using BLE
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT); // Release memory for Bluetooth Classic as we're not using it
  
  BLEDevice::init(DEVICE_TYPE);
//  BLEDevice::setMTU(512);

  // Setup security (optional)
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new securityBLECallbacks());
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setKeySize(16);
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  // Setup server, service and characteristic
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristic->setCallbacks(new messageReceivedBLECallback());
  pService->start();

  // Setup BLE advertising
  BLEAdvertising *pAdvertising;
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();
}
  
void setup() {
  Serial.begin(115200);

  loadUserSetup();

  // Setup comms
  setupBLE();

  // Get unique deviceID. Must be done after BLE setup to get a valid mac
  BLEAddress bdAddr = BLEDevice::getAddress();
  myDevice.setDeviceID(*bdAddr.getNative());
  Serial.print(F("Device ID: "));
  Serial.println(myDevice.deviceID);
}

void loop() {
  checkIncomingMessages();
  checkOutgoingMessages();
}
