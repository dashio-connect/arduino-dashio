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

#ifndef DashioBluefruitSPI_h
#define DashioBluefruitSPI_h

#include "Arduino.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "bluefruitConfig.h"
#include "DashIO.h"

#define MINIMUM_FIRMWARE_VERSION "0.6.6" // For LED behaviour
#define MODE_LED_BEHAVIOUR       "MODE"  // "DISABLE" or "MODE" or "BLEUART" or "HWUART"  or "SPI"  or "MANUAL"

class DashioBluefruit_BLE {
    private:
        bool printMessages;
        DashioDevice *dashioDevice;
        MessageData messageData;
        Adafruit_BluefruitLE_SPI bluefruit;
        void (*processBLEmessageCallback)(MessageData *messageData);

        void bleNotifyValue(const String& message);

    public:    
        DashioBluefruit_BLE(DashioDevice *_dashioDevice, bool _printMessages = false);
        void sendMessage(const String& message);
        void run();
        void setCallback(void (*processIncomingMessage)(MessageData *messageData));
        void begin(bool factoryResetEnable, bool useMacForDeviceID = true);
};

#endif
