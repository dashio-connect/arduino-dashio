#include <DashioESP.h>
#include "DashioProvisionESP.h"

#define DEVICE_TYPE "ESP8266_Dash"
#define DEVICE_NAME "Dash8266_ SAP"

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"
#define SOFT_AP_PWD    "" // Can be empty i.e. "" or must be a VALID password

// MQTT
#define MQTT_USER      "yourMQTTuserName"
#define MQTT_PASSWORD  "yourMQTTpassword"

#define MIN_SOFTAP_TIME_S 20

const char configC64Str[] PROGMEM =
"lVNLk9owDP4rjA89ZToQytJyIw8CQyAheEM7nR6yxJu4BJs6Do8y/PfKebC0Sw9MLor0SbK+Tzoj13DR4PsPDU3nngHWGRUFjdEA"
"TXvGvkiKpwWLwtXM2fq+U4QJ0lAuTxkBwNwLZkMXHGvOpODZxFJZRrujMITFHstOHgtIRqIc8FIUREPb6IgGnXZbQ7tIECbLJCss"
"kwSJwygrANuHuKSybDNl/AWCKaFJKoNIUo4G7Y/6U43weU7BxwCJPV+1TvlhRtlMNap6nhrMNbv3Cb6ehjZQ2+QZF+oRhOxaPmUb"
"qBHTKBvxLOOHvGxfF1LuBv6NqDBgDzSW6U1lDR3f9dP1vt7vwvQU3tm+ANtL1woq3rH9FVeWNalds6FfG/b8ubJcz6mMYWhVxtI1"
"cY033dqwwnBVathoZHiBZQdLJZIUGXAyAq2W9DfEusBxImgMAxVblqOBrivyQJXK0wzNiu0V0vlb7Fo3VdrgIiaiIWeVUkmayCZh"
"cRMwlLyV/79YLCKWl9uxPgFbqmWTHK2VOs1mWCBH60OrXhCVj1XE4Mc77ZrQu+qKgwCEBka6N9A3ojqfNURh7Hm0VV2fd+hSijV0"
"b8/lSzsUvwxjjB1nYqUptXfTn8fFzbkYwwD+doKsaV7ua/cfMksuH78PxcIj91HuNs2u/DuCEIbuXUmnpz54CadMvol73XzYjBci"
"bhqY9hzbwd0bfORKCkYlyIHQ7cGY46A+E+wE/rgyDYznjeXUN2OO4FLOKCZ7uiZLIosdlGKgnXagr1SLozzVZguMNWz61RBWCQ0p"
"OdQr/poEZA/m5fIH";

DashDevice    dashDevice(DEVICE_TYPE, configC64Str, 1);
DashWiFi      wifi;
DashSoftAP    soft_AP;
DashTCP       tcp_con(&dashDevice, true);
DashMQTT      mqtt_con(&dashDevice, true, true);
DashProvision dashProvision(&dashDevice);

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

    message = dashDevice.getKnobMessage("KB01", dialValue);
    message += dashDevice.getDialMessage("D01", dialValue);

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
            String message = dashDevice.getDialMessage("D01", dialValue);
            sendMessage(messageData->connectionType, message);
        }
        break;
    default:
        dashProvision.processMessage(messageData);
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
    
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword); // Setup MQTT host
    wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(buttonPin, INPUT);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashDevice.setup(WiFi.macAddress()); // Get unique deviceID
    Serial.print(F("Device ID: "));
    Serial.println(dashDevice.deviceID);

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
