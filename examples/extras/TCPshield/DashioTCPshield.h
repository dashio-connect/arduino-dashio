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

#ifndef DashioTCPshield_h
#define DashioTCPshield_h

#include "Arduino.h"
#include <Ethernet.h>
#include "DashIO.h"

class DashioTCPshield {
private:
    bool printMessages = false;
    DashioDevice *dashioDevice;
    MessageData dashioConnection;
    EthernetServer server;
    boolean alreadyConnected = false; // whether or not the client was connected previously
    void (*processTCPmessageCallback)(MessageData *messageData);

public:
    DashioTCPshield(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
    void setCallback(void (*processIncomingMessage)(MessageData *messageData));
    void sendMessage(const String& message);
    void begin(byte mac[]);
    void run();

//???        void setupmDNSservice();
//???        void updatemDNS();
};

#endif
#endif
