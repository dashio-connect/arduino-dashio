#ifndef DashioBluefruitSPI_h
#define DashioBluefruitSPI_h

#include "Arduino.h"
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include "bluefruitConfig.h"
#include "DashIO.h"

#define MINIMUM_FIRMWARE_VERSION "0.6.6" // For LED behaviour
#define MODE_LED_BEHAVIOUR       "MODE"  // "DISABLE" or "MODE" or "BLEUART" or "HWUART"  or "SPI"  or "MANUAL"

class DashioBluefruit_BLE {
    private:
        bool printMessages;
        DashioDevice *dashioDevice;
        DashioConnection dashioConnection;
        Adafruit_BluefruitLE_SPI bluefruit;
        void (*processBLEmessageCallback)(DashioConnection *connection);

        void bleNotifyValue(const String& message);

    public:    
        DashioBluefruit_BLE(DashioDevice *_dashioDevice, bool _printMessages = false);
        void sendMessage(const String& message);
        void checkForMessage();
        void setup(void (*processIncomingMessage)(DashioConnection *connection), bool factoryResetEnable);
};

#endif