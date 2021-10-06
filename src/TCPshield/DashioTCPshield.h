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
