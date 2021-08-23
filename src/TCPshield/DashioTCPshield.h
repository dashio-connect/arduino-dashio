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
        DashioConnection dashioConnection;
        EthernetServer server;
        boolean alreadyConnected = false; // whether or not the client was connected previously
        void (*processTCPmessageCallback)(DashioConnection *connection);
    
    public:    
        DashioTCPshield(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages = false);
        void setCallback(void (*processIncomingMessage)(DashioConnection *connection));
        void sendMessage(const String& message);
        void begin(byte mac[]);
        void checkForMessage();

//???        void setupmDNSservice();
//???        void updatemDNS();
};

#endif
#endif
