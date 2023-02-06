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
"rVTbctowEP2Vjp9x6jsmb5ZsQyZgGKOQzrR9MFiAJkZihJxAM/mnfkO/rGtsp5CkmXYmb6uzF0ln9+yjNkRD7fLr9452nYxRbU2H"
"YVpbJPpCwHrUypLl2qUWhyh2oyDSe6EV6I7hdXVkmLbu2Kbp9nA3cN2u1tEWgispiqsQUkg0mhgmgNtMUq6OGOphw/WiQPdd1NUd"
"3/P0wLd83fdREPl213H9EDJKztQOwsFUTBW0qkY3WyozVUoK6Jqy1VqlmWJCuzQuTNe0Db8JnogdA5xX140JGY+qKnSvgoKtKhBH"
"CYlSAJdCbjIFSDJOIjgf2sy2Ljxd0gXbHYvB6W6ek8OW/sl4YLlaP7/C6Wj71yUWhdjR63k+5lPKgUolS/rUeSbWDVDgBaapWx4C"
"OkPs6ihGPT2KfKfbRYYT2/ELYgc3o6vw45gdlBuWM3V4g1an5xnWh9N6YRiG5TsfwO6F9xd+YYLDq2aUR8GkMaLkpraG435tBLOw"
"nXxMmng8bIxwNrs9amCnDkem0DgNo3RatUPJYpTtY+jKlP0An2nBD1aS5VgU5YYDxRYQt1tDg2qkflhH4+XmOcQ87+s/trC6GwmZ"
"UwmFhITE2zVTtPXcrXjeOpLs/vAJFWXrbPGU5qfhRGZ8d5ylxaGe2SYOFdni7mRUmiRSHZHYt+VGmRTQxHPnq6IVO6l4gH/b9kno"
"CYfQTQaEJNnmpeLrhgbNwsKDlDRrqp9OBrWJCElONxbCruXFhgl7yse643pAJLIi3exhjGPkxQjb8GSxXLbf6EtK+QutxeiGHKU2"
"L5USPOLZvKB520zBX1Hw/5J8b5dBlV8/8bfPsfaOCt/YW5Uw636di8h6S0TOGe2J4DXfiPQb4eAY5PKo5fSeLeiUqnJbXT+sNArj"
"HB7hGaMPzUgvVym9B/Pp6Tc=";

// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);

// Create connection through Bluefruit
DashBluefruit_BLE  ble_con(&dashDevice, true);

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
        ble_con.sendMessage(dashDevice.getTextBoxMessage(TEMPERATURE_CONTROL_ID, String(t)));
    }

    float h = dht.readHumidity();
    if (isnan(h)) {
        Serial.println(F("DHT read fail"));
    } else {
        ble_con.sendMessage(dashDevice.getTextBoxMessage(HUMIDITY_CONTROL_ID, String(h)));
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
    ble_con.sendMessage(dashDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
}

void onIncomingMessageCallback(MessageData *messageData) {
    switch (messageData->control) {
        case status:
            processStatus();
            break;
        case button:
            if (messageData->idStr == BUTTON_CONTROL_ID) {
                fahrenheit = !fahrenheit;
                ble_con.sendMessage(dashDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
            }
            break;
        default:
            break;
    }
}

void setup(void) {
    Serial.begin(115200);

    dashDevice.name = DEVICE_NAME;
    ble_con.setCallback(&onIncomingMessageCallback);
    ble_con.begin(true, true);

    timer.every(1000, onTimerCallback); // 1000ms
    dht.begin();
}

void loop(void)
{
    timer.tick();
    ble_con.run();
}
