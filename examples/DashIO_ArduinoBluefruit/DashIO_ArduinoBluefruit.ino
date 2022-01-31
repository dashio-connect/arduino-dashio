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

// Create device
DashioDevice dashioDevice(DEVICE_TYPE);

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

void processConfig() {
    // Not enough space for text full config, so just do a basic config
    String configData = dashioDevice.getBasicConfigData(textBox, TEMPERATURE_CONTROL_ID, F("Temperature"));
    configData += dashioDevice.getBasicConfigData(textBox, HUMIDITY_CONTROL_ID, F("Humidity"));
    configData += dashioDevice.getBasicConfigData(button, BUTTON_CONTROL_ID, F("°C/F"));
    ble_con.sendMessage(dashioDevice.getBasicConfigMessage(configData));
}

void onIncomingMessageCallback(MessageData *messageData) {
    switch (messageData->control) {
        case status:
            processStatus();
            break;
        case config:
            processConfig();
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
    ble_con.begin(true);

    timer.every(1000, onTimerCallback); // 1000ms
    dht.begin();
}

void loop(void)
{
    timer.tick();
    ble_con.checkForMessage();
}
