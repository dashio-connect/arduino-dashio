#ifdef ARDUINO_AVR_UNO

#ifndef DashioBluno_h
#define DashioBluno_h

#include "Arduino.h"
#include "DashIO.h"

class DashioBluno {
    private:
        DashioDevice *dashioDevice;
        DashioConnection dashioConnection;
        void (*processBLEmessageCallback)(DashioConnection *connection);

        void bleNotifyValue(const String& message);

    public:    
        DashioBluno(DashioDevice *_dashioDevice);
        void sendMessage(const String& message);
        void checkForMessage();
        void setup(void (*processIncomingMessage)(DashioConnection *connection));
};

#endif
#endif
