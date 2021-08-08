#include "DashioBluefruitSPI.h"

DashioBluefruit_BLE::DashioBluefruit_BLE(DashioDevice *_dashioDevice, bool _printMessages) : dashioConnection(BLE_CONN),
                                                                                             bluefruit(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
                                                                                                       BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
                                                                                                       BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

void DashioBluefruit_BLE::sendMessage(const String& writeStr) {
    if (bluefruit.isConnected()) {
        bluefruit.print(writeStr);
        bluefruit.flush();
        
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(writeStr);
            Serial.println();
        }
    }
}

void DashioBluefruit_BLE::checkForMessage() {
    while(bluefruit.available()) {
        char data;
        data = (char)bluefruit.read(); // Read individual characters.
    
        if (dashioConnection.processChar(data)) {
            if (printMessages) {
                Serial.println(dashioConnection.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(dashioConnection.control)));
            }

            switch (dashioConnection.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            default:
                processBLEmessageCallback(&dashioConnection);
                break;
            }
        }
    }
}

void DashioBluefruit_BLE::setup(void (*processIncomingMessage)(DashioConnection *connection), bool factoryResetEnable) {
    processBLEmessageCallback = processIncomingMessage;

  // Initialise the module
    Serial.print(F("Initialising Bluefruit LE module: "));
    if (!bluefruit.begin(VERBOSE_MODE)) {
        Serial.print(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring"));
        while(1);
    } else {
        Serial.println(F("OK!"));
    }

    // Perform a factory reset to make sure everything is in a known state
    if (factoryResetEnable) {
        Serial.println(F("Performing a factory reset: "));
        if (!bluefruit.factoryReset()) {
        Serial.print(F("Couldn't factory reset"));
        while(1);
        }
    }
  
    bluefruit.echo(false); // Disable command echo from Bluefruit
    Serial.println("Requesting Bluefruit info:");
    bluefruit.info(); // Print Bluefruit information
    bluefruit.verbose(false);  // debug info is a little annoying after this point!
    
    // LED Activity command is only supported from 0.6.6
    if (bluefruit.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
        // Change Mode LED Activity
        Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
        bluefruit.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    }

  // Get deviceID for mac address
    bluefruit.println("AT+BLEGETADDR");
    delay(1000); // Wait a while for reply
    dashioDevice->deviceID = "";
    while(bluefruit.available() > 0) {
        char c = bluefruit.read();
        if ((c == '\n') || (c == '\r')) { // Before the OK
            break;
        }
        dashioDevice->deviceID += c;
    }
    Serial.print(F("DeviceID: "));
    Serial.println(dashioDevice->deviceID);

    // Set local name name
    Serial.print(F("Set local name to DashIO_"));
    Serial.println(dashioDevice->type);
    String commandMsg = F("AT+GAPDEVNAME=DashIO_");
    commandMsg += dashioDevice->type;
    bluefruit.sendCommandCheckOK(commandMsg.c_str());
    
    // Set Bluefruit to DATA mode
    Serial.println(F("Switching to DATA mode!"));
    bluefruit.setMode(BLUEFRUIT_MODE_DATA);
    
    Serial.println();
}
