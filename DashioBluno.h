#ifdef ARDUINO_AVR_UNO

#ifndef DashioBluno_h
#define DashioBluno_h

#include "Arduino.h"
#include "DashIO.h"

class DashioBluno {
private:
    DashioDevice *dashioDevice;
    MessageData messageData;
    void (*processBLEmessageCallback)(MessageData *messageData);

    void bleNotifyValue(const String& message);

public:
    DashioBluno(DashioDevice *_dashioDevice);
    void sendMessage(const String& message);
    void checkForMessage();
    void setCallback(void (*processIncomingMessage)(MessageData *messagrData));
    void begin(bool useMacForDeviceID = true);
};

#endif
#endif
