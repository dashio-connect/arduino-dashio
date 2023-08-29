/*
 * Dash demonstration project for toggling an LED on an ESP32.
 * Communicates via BLE and MQTT connections.
 * Uses LED on pin 13, but can be changed.
 * Button press on dash app toggles LED and increments counter.
 * Counter value displayed in a TextBox.
 * Uses the serial monitor to show what is going on (115200 baud).
 */

#include "DashioESP.h"

const char configC64Str[] PROGMEM =
"lVPbctowEP0VRq/1NFzTJm/4EqAYQ2yVdKb0wRdha2IkRpa5hOHfu5LthITmoaNhOOweLdqze07INV10//uPgabe3KxQ4Np+hbDz"
"CwM6obKkCbpH0Rc7zV5+7Pz9rT2kq5vBrp867BEZKOZMCp5PbGDhdgci21AQJnXAXqpAyags4BdASWVOFFN/GygjNM2kH0rK0X37"
"a2egTk1b8IJCnAHdm3uOuk0OcpjTVIUsx8OOD8E1F5tQvpGOzb2rqltBYlroim0DPUcJPm7J28U9TWT2eqnf/T741jHQ4aqcTvSh"
"85wXZBolcxYQBiJJUZIzaGdPahFnw0UNHO9nhdz5qALDpd1obuGab7k1sJfLJ61+IY9aLnMYTCwlthT5LDw8gOYBfYFMpwudpIIm"
"Fs/LDQOVu/DmIoMRVJHqWQZi5eaV0nk/tWpIqrTJRUIE8LhQf5qH8XOTeU5Z0iSeMipJnfiUjEXICr0K8VHrHdfMUUhZEXHBL9ah"
"vqi3wuSHpugsFBym9T55VVi17/M9NNbrXVAvRLo1EIWOvXCjx80ZQdWghrUFrLGP68Uf+YtxBU2MvUsPOPt59hDfRsHUWt3EZHWz"
"dLuDhatM0MxpPBmNXfhgiPH1uunDppvWSITHD3YxtV2iUkrOHBZGOUmacXH2qgFJaLlpBXkoScvMS/Ivh32wUa8N59psn7jq2jC9"
"O3Uqv1XT+V9vXMjtqqe1zDKPtOgmHtWusB7ACyeUkB2NSUBkuVXtPmJsmK56F6ysrXNLSvb12q5Tn+wAns9/AQ==";

char WIFI_SSID[] = "yourWiFiSSID";
char WIFI_PASSWORD[] = "yourWiFiPassword";

char MQTT_USER[] = "yourMQTTuserName";
char MQTT_PASSWORD[] = "yourMQTTpassword";

int ledPin = 13;
const char *BUTTON_ID = "B01";
const char *TEXT_BOX_ID = "T01";

DashDevice dashDevice("ESP32_Type", configC64Str, 1);
DashBLE    ble_con(&dashDevice, true);
DashMQTT   mqtt_con(&dashDevice, true, true);
DashWiFi   wifi;

bool buttonValue = false;
int counter = 0;

void sendMessage(const String& message) {
    // Send all messages through both connections in this simple example.
    ble_con.sendMessage(message);
    mqtt_con.sendMessage(message);
}

void sendData() {
    String message = dashDevice.getButtonMessage(BUTTON_ID, buttonValue);
    message += dashDevice.getTextBoxMessage(TEXT_BOX_ID, String(counter));
    sendMessage(message);
}

void processButtonPress() {
    counter += 1;
    buttonValue = !buttonValue;
    digitalWrite(ledPin, buttonValue);
    sendData();
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        sendData();
        break;
    case button:
        if (messageData->idStr == BUTTON_ID) {
            processButtonPress();
        }
        break;
    }
}

void setup() {
    pinMode(ledPin, OUTPUT);
    
    Serial.begin(115200);
    delay(1000);
    
    dashDevice.setup(wifi.macAddress(), "Toggle Button"); // unique deviceID, and device name

    mqtt_con.setup(MQTT_USER, MQTT_PASSWORD);
    mqtt_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&mqtt_con);
    wifi.begin(WIFI_SSID, WIFI_PASSWORD);

    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin();
}

void loop() {
    wifi.run();
    ble_con.run();
}
