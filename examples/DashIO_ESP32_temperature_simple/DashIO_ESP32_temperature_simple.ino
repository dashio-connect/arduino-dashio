/*
 * Simple dash demonstration project for a temperature sensor attached to an ESP32.
 * Compatible with Dallas one-wore temperature sensors.
 * Communicates via Dash MQTT connection.
 * Demonstrates TimeGraph control, which updates every 10 minutes.
 * Requires the dash MQTT server for continuous temperature storage.
 * Sensor attached to pin 13, but can be changed.
 * Uses the serial monitor to show what is going on (115200 baud).
 */

#include "DashioESP.h" // Dash ESP core library
#include "OneWire.h" 
#include "DallasTemperature.h" //Arduino Library for Dallas Temperature ICs. Supports DS18B20, DS18S20, DS1822, DS1820

#define DEVICE_TYPE "TempMonitor"
#define DEVICE_NAME "Temperature"

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

#define GRAPH_UPDATE_SECONDS (60 * 10) // Every 10 mins

const char configC64Str[] PROGMEM =
"jVPbbuIwEP2Vys/JyiGBQt/iONCqELrBpSut9iEQFyxCjBynhVb9p/2G/bId59JC2UqrvMzlzFF8Zs4rGpMxuvr5y0K30ZTU0WxM"
"4zpi4Q8G0SsqS5GiK9Tz/aDfdYlNe8S3vbDv2IRSbLvukOKB5w06IUEWWspcK5ndUBhhBDtQ2iWK57qqBHWlzIUuIP3zO4BMC51x"
"A+fbHVeJLhWH6pqL1VrHiRYSXeFvTtfxLgcN+E4WAuo5DEXTKDQcfK/9TKxMKQgjFsZQfJRqm2gDup9AemjHWlL4NcWXoqiYINss"
"UnbY8Q/WZ5HqdYN2LLQ/J1hmsuC3i3Saz3gOMmlV8jdQj940Mk78uyYIo/s6Gk9HdeDPaat6wBp8MG4COp8/VPoX+lDJQ6YxDeOZ"
"0VirbJLshyD1TLxAz+taaKVEGsis3OagbKdjoWINuteV+r8slJfbd4hzuqtmM4aaSJVyBUCpoBHLQ5JdkKzkbXuzytMvu23jYS30"
"yQRTSV5Up7A81NI1SJIly83RGTRDzKRE7lvCKcyv+EXM00+AM2KjRCyf4Y2uewT9kMvpW0jA46NkWy1b5hzVS/MbQwTXMWtsMIrv"
"ro994PWGQScYEtsNKbW9Hu7bfRpimxLSw/6lGw6Jb47N34tiIkB6B58qfUPZ6B2Q7GFZ3X955NP9uz13gLtfmaUiGycLnp266two"
"ZyYwzoLvmCPW9RqqSlRuSaJAzO7HowwA/4892keaAQdjbFQmjEW1toSNGnMEQ7DEK0r5k1jyGdflDv5g8p0xVB0trepzwZ+bw31c"
"xfwJwre3vw==";

// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);

// Create Connections
DashMQTT mqtt_con(&dashDevice, true, true);
DashWiFi wifi;

// Create Control IDs
const char *TEMPTB_ID = "TB01";
const char *GRAPH_ID = "IDTG";

OneWire oneWire(13); // Temperature sensor connected to pin 13
DallasTemperature tempSensor(&oneWire);

int graphSecondsCounter = 0;
float sampleTempC;
float temperatureC;
float tempSum = 0;

bool oneSecond = false;

void processStatus() {
    String message = dashDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));
    message += dashDevice.getTimeGraphLine(GRAPH_ID, "L1", "Avge Temp", line, "red", yLeft);
    mqtt_con.sendMessage(message);
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus();
        break;
    default:
        break;
    }
}

void setTemperatureEverySecond(float temperature) {
    String messageToSend = "";
    if (temperature > -100) { // i.e. no an error
        temperatureC = temperature;
        messageToSend = dashDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

        // Now calculate the average temperature and send every 10 minutes. Dash server will store this.
        tempSum += temperatureC;
        graphSecondsCounter++;
        if (graphSecondsCounter >= GRAPH_UPDATE_SECONDS) {
            float tempFiltered = tempSum / GRAPH_UPDATE_SECONDS;
            messageToSend += dashDevice.getTimeGraphPoint(GRAPH_ID, "L1", tempFiltered);

            String text = "The temperature is " + String(tempFiltered) + "°C";

            graphSecondsCounter = 0;
            tempSum = 0;
        }
    } else {
        messageToSend = dashDevice.getTextBoxMessage(TEMPTB_ID, "Err");
    }
    mqtt_con.sendMessage(messageToSend);
}

void oneSecondTimerTask(void *parameters) {
    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        oneSecond = true;
    }
}

void setup() {
    Serial.begin(115200);

    dashDevice.setup(wifi.macAddress(), DEVICE_NAME); // unique deviceID
    
    mqtt_con.setup(MQTT_USER, MQTT_PASSWORD);
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.addDashStore(timeGraph, GRAPH_ID);
        
    wifi.attachConnection(&mqtt_con);
    wifi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Setup 1 second timer task for measuring the temperature (overkill)
    xTaskCreate(
        oneSecondTimerTask,
        "One Second",
        1024, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );

    // Start temperature sensor conversion
    tempSensor.setWaitForConversion(false);
    tempSensor.begin();
    tempSensor.requestTemperaturesByIndex(0);
}

void loop() {
    wifi.run();

    if (oneSecond) {
        oneSecond = false;
    
        if (tempSensor.isConversionComplete()) {
            sampleTempC = tempSensor.getTempCByIndex(0);
            tempSensor.requestTemperaturesByIndex(0);
        }
        setTemperatureEverySecond(sampleTempC);
    }
}
