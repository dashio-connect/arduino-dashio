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
"vVTNctowEH6Vjs52aoxxDDcMzs+0QCYx7aGTg7AX0CBLjCyn0Azv1Gfok3VlZAppkqYznfqk/bS72v12Pz8SyqkqStL7cu+QGYeB"
"FAIyzaQgvUeigObT6fWQ9EgIgYdf2511aNudt7ttFzzadaHjgR/kWUa7QBxSgnpgGZwGtf4Q9FUx/STEfzVkh7VWWktxqWS1bqqv"
"EWM82nMiKLaUk55WFTgkk0IryetnLuJp6rXw7SWwxVLfUmyZ9LyzVsc7b3cdwtB5TAtA17EUpkg5nw8klwqRSwUgDCQaZESVlAZa"
"UwVC10/E3YHXCZO+G3XiczeIwtDtR37kRlHcT6L2edCJhhihYaPRe/D+whhMc/Pmj+9H9o0s2X4iJJ6k6WRkKGO5Xh6q9h2yabwO"
"YOCQ7RPQP4uCxA1294YMLitlqcvBDA1nP2cLM3dRFcMa+sTgK/q0do2PBZDizLYec5qtyCm7b2w904rHq4XIGxrH9GH7LuYVHF+m"
"ioqy5jXbYl/2RqocVBP3eYkLRI5vJuIwdcQav1tchj0yopsLLPiOfUO6W77Nmhq6Y7n5bazHl8/WU18eZQxPViiFYg2K6kqZKpFd"
"fKAqzK62UDJLzLUHmpqbNSBmUjmjvJkTU3txWhsecNk+yoU1F4qul/a8EnJmj5zOoMlQgKjssQSOyWSzAyVnyJs1zFJiq2BHzWUJ"
"H2b5RNyBeFZOaTK6qeU0l6qgZp/Hk3HysrxWszzdruGX39/rZo1UsLKWhbcvuM/ZwohkkIzT5PZITaf0v6ipSjBdGtafyit4Tl7e"
"y/Jy3kLZ1XR0PXwTZ0E39Pz/zdlVVbCc6e2/Iyx89X+kWQGXh/3d/QQ=";

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
