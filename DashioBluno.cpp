#ifdef ARDUINO_AVR_UNO

#include "DashioBluno.h"

DashioBluno::DashioBluno(DashioDevice *_dashioDevice) : dashioConnection(BLE_CONN) {
    dashioDevice = _dashioDevice;
}

void DashioBluno::sendMessage(const String& writeStr) {
    Serial.print(writeStr);
}

void DashioBluno::checkForMessage() {
    while(Serial.available()) {
        char data;
        data = (char)Serial.read();

        if (dashioConnection.processChar(data)) {
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

void DashioBluno::setup(void (*processIncomingMessage)(DashioConnection *connection)) {
    processBLEmessageCallback = processIncomingMessage;

    Serial.begin(115200); //initialise the Serial
    
    // Set peripheral name
    delay(350);
    Serial.print("+++"); // Enter AT mode
    delay(500);
    Serial.println("AT+NAME=DashIO_" + dashioDevice->type); // If the name has changed, requires a RESTART or power cycle
    delay(500);
    Serial.println("AT+NAME=?");
    delay(500);
    
    // Get deviceID for mac address.
    while(Serial.available() > 0) {Serial.read();} // But first, clear the serial.
    Serial.println("AT+MAC=?");
    delay(1000); // Wait a while for reply
    dashioDevice->deviceID = "";
    while(Serial.available() > 0) {
        char c = Serial.read();
        if ((c == '\n') || (c == '\r')) { // Before the OK
            break;
        }
        dashioDevice->deviceID += c;
    }
    
    Serial.println("AT+EXIT");
    delay(1000);
    
    Serial.print(dashioDevice->getWhoMessage()); // In case WHO message is received when in AT mode
}

#endif
