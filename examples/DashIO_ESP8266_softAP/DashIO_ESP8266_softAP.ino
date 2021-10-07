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

DashioDevice    dashioDevice(DEVICE_TYPE);
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

void processConfig(ConnectionType connectionType) {
    String message((char *)0);
    message.reserve(2048);

    message += dashioDevice.getConfigMessage(DeviceCfg(1, "name, wifi, dashio")); // One Device Views

    DialCfg first_dial_control("D01", "DV01", "Dial", {0.24, 0.14, 0.54, 0.26});
    message += dashioDevice.getConfigMessage(first_dial_control);

    KnobCfg aKnob("KB01", "DV01", "Knob", {0.24, 0.42, 0.54, 0.26});
    message += dashioDevice.getConfigMessage(aKnob);

    TCPConnCfg tcpCnctnConfig(wifi.ipAddress(), TCP_PORT);
    message += dashioDevice.getConfigMessage(tcpCnctnConfig);
    
    MQTTConnCfg mqttCnctnConfig(dashioProvision.dashUserName, DASH_SERVER);
    message += dashioDevice.getConfigMessage(mqttCnctnConfig);

    DeviceViewCfg deviceView("DV01", "Dial & Knob", "up", "black");
    message += dashioDevice.getConfigMessage(deviceView);
    
    sendMessage(connectionType, message);
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
    case config:
        processConfig(messageData->connectionType);
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
