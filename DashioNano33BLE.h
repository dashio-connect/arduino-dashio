#ifdef ARDUINO_ARDUINO_NANO33BLE //ARDUINO_ARCH_NRF52840

#ifndef DashioNano33BLE_h
#define DashioNano33BLE_h

#include "Arduino.h"

#include "DashIO.h"
#include <ArduinoBLE.h>

// Create 128 bit UUIDs with a tool such as https://www.uuidgenerator.net/
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class DashioBLE {
private:
    bool printMessages;
    DashioDevice *dashioDevice;
    static MessageData messageData;
    BLEService bleService;
    BLECharacteristic bleCharacteristic;

    static void onBLEConnected(BLEDevice central);
    static void onBLEDisconnected(BLEDevice central);
    static void onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic);

public:
    void (*processBLEmessageCallback)(MessageData *connection);

    DashioBLE(DashioDevice *_dashioDevice, bool _printMessages = false);
    void sendMessage(const String& message);
    void run();
    void setCallback(void (*processIncomingMessage)(MessageData *connection));
    void begin();
    bool connected();
    void end();
    String macAddress();
};

#endif
#endif
