  // Arduino with Adafruit Bluefruit LE SPI Friend and DHT Temperature and Humidity Sensor

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
//#include <Adafruit_BluefruitLE_UART.h>
#include <SPI.h>
#include <DHT.h>
#include <arduino-timer.h>
#include "bluefruitConfig.h"
#include "DashioBluefruitSPI.h"

// Temperature and humidity sensor
#define DHTPIN 2 // pin that temp and humid sensor is connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11 // DHT 11 
#define DHTTYPE DHT22 // DHT 22  (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

#define DEVICE_TYPE "TempHumidBLEType"
#define DEVICE_NAME "TempHumidBLE"

const char configC64Str[] PROGMEM =
"zVTbctowEP2Vjp5xyj2GN9uYJNMCmcSkD20fhL2AJrLEyHICZfinfkO/rCvbIqa5tNPpQ+HFe6yzqz17vHuSgsgzMvz8tUE4XQCv"
"njVstS+3YMI9iaXQSvKrERmSKJxcN1ukQTZUgdAF5g+CZq8feo7b88+drtvvO57bdh3X9b3Q7Zx3e+4IGblgGhMSfNRMczDZIN2A"
"ojpXgOga2Gqtb6hmkgybZ62e+VeHr2XGEBem3CyKZhNS3tLjbGXAIJxG4Q2CS6lSqhGZzqYhxjvLtHnx6gpilhXJMLpfJNFuA0+M"
"R5bo9fEW3QbZPk8Rc5nBh0UyE7cgEjLUKodD41Sqy/nkavTvtLrMU5YwvXtJqP9RpLP+KzKhv1K6qZy2yLWW4kLJ/BQpnSeXy0By"
"qbDuhQIQWLiu8NifR4XAJScUdMGhKtMgUljuhCopxV8N4i1PYpYf34Mv78fkDf1fkNaMxEyjZJ4q2X5JSZSXYd9TmhYTkAKIUXGl"
"6GZtVeMQSCEgLuvviQKazOdFq/2w28Rfx/F7XscZdwYdJ2x6AyfsNcN2dxQE3sCMNAP1wGI4JbV+Q3pUTP9Cab9JOaCZhFxU104Y"
"tTsnxmHlqgoopyq1y4ilcFFrNQFzTex2yVamU5GnowK6Y/CIZ1oHe6YCzArTik/odozmuWXfUMVWGweRrdEPaJI8FdY0mKwETKJT"
"t/2hY0wp/34lEuu+KX3YvfN5DvalVAmo2bHkE2YZn9YoanXaYjdo7FIkE/mcxve15VCvGykqssLp8a5cVvgmMgdxpT/7JOovX2fW"
"hOufmLG+wY0nM+DoQWnnmHGGfR2HrUp/2iHnCZN3LMuPHoAH/Dw/ylURHn4C";

// Create device
DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);

// Create connection through Bluefruit
DashioBluefruit_BLE  ble_con(&dashioDevice, true);

// Create controls. This simple example only requires the controlID for each control
const String TEMPERATURE_CONTROL_ID = "TEMP01";
const String HUMIDITY_CONTROL_ID = "HUMID01";
const String BUTTON_CONTROL_ID = "FBUT01";
bool fahrenheit = off; // Used for the button control

// Global variables used throughout
auto timer = timer_create_default();
DHT dht(DHTPIN, DHTTYPE);

static bool onTimerCallback(void *argument) {
// onTimerCallback is not an interrupt, so it is safe to read data and send messages from within.
// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
// check if returns are valid, if they are NaN (not a number) then something went wrong!

    float t = dht.readTemperature(fahrenheit);
    if (isnan(t)) {
        Serial.println(F("DHT read fail"));
    } else {
        ble_con.sendMessage(dashioDevice.getTextBoxMessage(TEMPERATURE_CONTROL_ID, String(t)));
    }

    float h = dht.readHumidity();
    if (isnan(h)) {
        Serial.println(F("DHT read fail"));
    } else {
        ble_con.sendMessage(dashioDevice.getTextBoxMessage(HUMIDITY_CONTROL_ID, String(h)));
    }

    return true; // to repeat the timer action - false to stop
}

String getTemperatureSymbol() {
    String symbol;
    if (fahrenheit) {
        symbol = "F";
    } else {
        symbol = "°C";
    }
    return symbol;
}
  
// Process incoming control mesages
void processStatus() {
    // Control initial condition messages (recommended)
    ble_con.sendMessage(dashioDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
}

void onIncomingMessageCallback(MessageData *messageData) {
    switch (messageData->control) {
        case status:
            processStatus();
            break;
        case button:
            if (messageData->idStr == BUTTON_CONTROL_ID) {
                fahrenheit = !fahrenheit;
                ble_con.sendMessage(dashioDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
            }
            break;
        default:
            break;
    }
}

void setup(void) {
    Serial.begin(115200);

    dashioDevice.name = DEVICE_NAME;
    ble_con.setCallback(&onIncomingMessageCallback);
    ble_con.begin(true, true);

    timer.every(1000, onTimerCallback); // 1000ms
    dht.begin();
}

void loop(void)
{
    timer.tick();
    ble_con.checkForMessage();
}
