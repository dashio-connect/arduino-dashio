#if defined ARDUINO_ARCH_AVR

#include "DashioTCPshield.h"

DashioTCPshield::DashioTCPshield(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages) : server(_tcpPort), dashioConnection(TCP_CONN) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

void DashioTCPshield::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processTCPmessageCallback = processIncomingMessage;
}

void DashioTCPshield::sendMessage(const String& message) {
    if (alreadyConnected) {
        server.print(message);

        if (printMessages) {
            Serial.println(F("---- TCP Sent ----"));
            Serial.println(message);
            Serial.println();
        }
    }
}

void DashioTCPshield::begin(byte mac[]) {
    Ethernet.begin(mac); // start listening for clients
    server.begin(); // Open serial communications and wait for port to open:
    
    Serial.print("IP address: ");
    Serial.println(Ethernet.localIP());
}

void DashioTCPshield::run() {
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
                    processTCPmessageCallback(&dashioConnection);
                    break;
                }
            }
        }
    }
}

#endif
