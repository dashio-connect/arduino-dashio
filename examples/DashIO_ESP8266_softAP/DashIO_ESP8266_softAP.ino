#include <DashioESP.h>
#include "DashioProvisionESP.h"

#define DEVICE_TYPE "ESP8266_DashIO"
#define DEVICE_NAME "DashIO8266"

#define TCP_PORT 5050
#define MQTT_BUFFER_SIZE 2048

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"
#define SOFT_AP_PWD    "" // Can be empty i.e. "" or must be a VALID password

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

#define MIN_SOFTAP_TIME_S 20

const char configC64Str[] PROGMEM =
"lZRbb9owFMe/SuWHPUUooZe1vHFp0VQuVUaZpqkPJj4jFo6d2U6BIr77jhMnhUKlTULIPj7X3z/2jmQgC0M6v14CIugChF9b2Nie"
"2oDfZjT3q0VhrZJDrYpji98sNc1Tv15JtXDLHTF2K4B0yGQaj7sjEpBESauV+DZA42MvjNBkQLKpFNupjEEANehvdQGu9oZ0ojAM"
"SE41SFsGDeZlkAY2p6JA3694brktyzxiYTxMgS9TG1PLFemErfaN93hShqNNouds+uRKp2o95nLsClU1t7VPE33VrgbqK6E0RsbA"
"MJJxKh6UEGptyqI+3Jlrx5/gjtF3zZlNm3zXVwHZnFRpozXj2Fm4d9T/WNtXUkJStbsjqTL2OR5hWkZN2nJ/XLUwNiCFAT2hmZuf"
"7KsWKvi5hoSbMsFl0EjR68YfdBiURP+f9gArnadd0uGiQTHUAJJ8psIp8whp5IpLC/qEpiyyBeiDJP37yew+Pqvmv5IvJLfG8TsU"
"IcHKha6vRZIfC5IrbUnnOnTAeN5lTINxKaK7diu6uW3hLwrvnCBUUJ35NAxeeQJzDutKosRqgf0+oBrf+RtCvQzdHCgAzl1ksh4E"
"h64MGBZ9UK+SyGXqrZaS1cB6Ti9vV5qBnjbZ3m2184+U29r7xNYkpMmKHMt/8eXC37mm/kxTacpPKNkiyepk5mLwWTnTXH30edw7"
"negWYePw/nN/zsm+gYry/OZLJw3CGhxyjvau6QyGB0+UwacmsaqW1wiONGqRuK509nt4xeswUku/pQXjas5NUd2zl/1f";

DashioDevice    dashioDevice(DEVICE_TYPE, configC64Str);
DashioWiFi      wifi;
DashioSoftAP    soft_AP;
DashioTCP       tcp_con(&dashioDevice, TCP_PORT, true);
DashioMQTT      mqtt_con(&dashioDevice, MQTT_BUFFER_SIZE, true, true);
DashioProvision dashioProvision(&dashioDevice);

int dialValue = 0;

const int buttonPin = 5;
int oldButtonState = HIGH;

auto timer = timer_create_default();
bool oneSecond = false; // Set by hardware timer every second.
int softAPtimer = 0;
bool softAPactive = false;

// Timer Interrupt
static bool onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == TCP_CONN) {
        tcp_con.sendMessage(message);
    } else {
        mqtt_con.sendMessage(message);
    }
}

void processStatus(ConnectionType connectionType) {
    String message((char *)0);
    message.reserve(1024);

    message = dashioDevice.getKnobMessage("KB01", dialValue);
    message += dashioDevice.getDialMessage("D01", dialValue);

    sendMessage(connectionType, message);
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData->connectionType);
        break;
    case knob:
        if (messageData->idStr == "KB01") {
            dialValue = messageData->payloadStr.toInt();
            String message = dashioDevice.getDialMessage("D01", dialValue);
            sendMessage(messageData->connectionType, message);
        }
        break;
    default:
        dashioProvision.processMessage(messageData);
        break;
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);
}

void startSoftAP() {
    Serial.println("Switch to soft AP");
    softAPactive = true;
    wifi.end();
    
    if (soft_AP.begin(SOFT_AP_PWD)) {
        softAPtimer = 0;
    } else {
        Serial.println(F("Failed to start soft AP"));
        startWiFi();
    }
}

void startWiFi() {
    Serial.println("Switch to WiFi");
    softAPactive = false;
    soft_AP.end();
    
    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword); // Setup MQTT host
    wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(buttonPin, INPUT);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashioDevice.setup(WiFi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashioDevice.deviceID);

    tcp_con.setCallback(&processIncomingMessage);
    mqtt_con.setCallback(&processIncomingMessage);

    soft_AP.attachConnection(&tcp_con);
    
    wifi.attachConnection(&tcp_con);
    wifi.attachConnection(&mqtt_con);

    startWiFi();

    timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    timer.tick();

    if (softAPactive) {
        soft_AP.run();
        
        if (oneSecond) { // Tasks to occur every second
            oneSecond = false;
            if ((softAPtimer >= MIN_SOFTAP_TIME_S) && (soft_AP.isConnected() == false)) {
                startWiFi();
            }
            softAPtimer += 1;
            Serial.print("soft AP Timer: ");
            Serial.println(softAPtimer);     
        }
    } else {
        wifi.run();

        int buttonState = digitalRead(buttonPin); // read the button pin
        if (oldButtonState != buttonState) {
            oldButtonState = buttonState;
    
            if (buttonState) {
                startSoftAP();
            }
        }
    }
}
