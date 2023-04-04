#ifndef DashioBluefruitSPI_h
#define DashioBluefruitSPI_h

#include "Arduino.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "bluefruitConfig.h"
#include "Dashio.h"

#define MINIMUM_FIRMWARE_VERSION "0.6.6" // For LED behaviour
#define MODE_LED_BEHAVIOUR       "MODE"  // "DISABLE" or "MODE" or "BLEUART" or "HWUART"  or "SPI"  or "MANUAL"

class DashioBluefruit_BLE {
    private:
        bool printMessages;
        DashioDevice *dashioDevice;
        MessageData messageData;
        Adafruit_BluefruitLE_SPI bluefruit;
        void (*processBLEmessageCallback)(MessageData *messageData);
        void processConfig();

        void bleNotifyValue(const String& message);

    public:    
        DashioBluefruit_BLE(DashioDevice *_dashioDevice, bool _printMessages = false);
        void sendMessage(const String& message);
        void run();
        void setCallback(void (*processIncomingMessage)(MessageData *messageData));
        void begin(bool factoryResetEnable, bool useMacForDeviceID = true);
};

#endif
