/*
 MIT License

 Copyright (c) 2021 Craig Tuffnell, DashIO Connect Limited

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

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
