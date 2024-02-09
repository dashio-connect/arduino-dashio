#include "DashioSAMD_NINA.h"
#include "DashioProvisionSAMD.h"

#include <arduino-timer.h>

//#define NO_TCP
//#define NO_MQTT

#define DEVICE_TYPE "SAMD_NINA_Type"
#define DEVICE_NAME "SAMD_NINA_Dash"

// WiFi
#define WIFI_SSID     "yourWiFiSSID"
#define WIFI_PASSWORD "yourWiFiPassword"

// MQTT
#define MQTT_USER     "yourMQTTuserName"
#define MQTT_PASSWORD "yourMQTTpassword"

// BLE
#define MIN_BLE_TIME_S 10
  
const PROGMEM char *configC64Str = 
"dVRdd6IwEP0rnjzsy3J2RWt3j2/yUetWwQKl27O7D1EipEKwEAra43/fCQRr1Z48MCR3ZpJ7Z+YNTbUpGv75p6A7y9Yay50aDlhv"
"qChogIZIVd1sp790HfuF+evyOSlsZoZIQXlMA5KZDC9iAkCeFURBy5TxLI0nBnhODFfACAtsFm9t5pCY4Jyg4QrHOWATXKGh2u0q"
"aIMzwnjtZPhdFbwyEvg4LgD8A8455TGYyKu/CooIDSPuYE5TNOx+u+qJJWHzNKewzwBu2ZYpbhCl5YyymUgnU29b1CHG4ArWQEFr"
"li70NE4z8J/hEK6FIcQCZzdpHKdl7tavPgSCgxbtbjLKws44I4SBS0kDHh3Cqz/FUlB1lljtX/evr+pILt/Wz3TNMQRIKLyhuwdJ"
"PPO314hjTJzGmI3m0jCth8aa2uPGGPlGK6XuSbw+lYbh+4+1vLlMptmOYTpCqSXPYiDpBiR06Q7O+kB9mNEAXlgkLEfDXk+wCWI1"
"O63orEgOEPVjDUg5RWgtzYC5lq3HiHLSnqxDFrQHmlC92f8U62WY5XXRLLfAkUjZOuPlGr0XjEFx3PnSuQNRpX9dQlpaXUjXHp1F"
"Fxw4oD0w0j+CvhMllKXwbAsnIuvDBu1rsUayu/RbxzvuqYeJwcLnp1/3PnW+BndjauqDpIRrbEcVzWe18FAr4meKFyTO28qwLXTa"
"Y6JUqmO3JobsrYvNVZ0gTtvrPTFsok/76qQLByqs3oXeklc6Clj/W0Wi4QxIHciMxxvH3aNeaJu6LcbO/LYhWPM865jgySxKSme9"
"cHe6v7Oo//d7t1rP7yF1ulq10jswtk7Z1ESzF5yn7GSwpax1eyJiEKBLxJ4w0lPFOuf3Ez7Ph9K1nA2cVBxweoRZSM5mS1+Mlt6l"
"2TL4UJZTcbmOVsSLujw1byxniH4Dk+MNBeSVLolLeLEBNAMnpaQrqgQ4jxRPnyuze89DdbsbNdSnpJQtvwod8grmfv8f";

DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);
DashProvision dashProvision(&dashDevice);

// dash comms connections
DashWiFi wifi;
DashBLE  ble_con(&dashDevice, true);
#ifndef NO_TCP
DashTCP  tcp_con(&dashDevice, true);
#endif
#ifndef NO_MQTT
DashMQTT mqtt_con(&dashDevice, true, true);
#endif

// variables for button
const int buttonPin = 2;

// Create Control IDs
const char *SLIDER_ID = "IDS";
const char *BUTTON_ID = "IDB";
const char *CHART_ID = "IDG";

auto timer = timer_create_default();
bool oneSecond = false; // Set by hardware timer every second.
ButtonMultiState toggle = off;
unsigned int bleTimer = MIN_BLE_TIME_S; // Start off in WiFi mode
bool bleActive = true; // Start off in WiFi mode

void sendMessage(ConnectionType connectionType, const String& message) {
    if (connectionType == BLE_CONN) {
        ble_con.sendMessage(message);
    } else if (connectionType == TCP_CONN) {
#ifndef NO_TCP
        tcp_con.sendMessage(message);
#endif
    } else {
#ifndef NO_MQTT
        mqtt_con.sendMessage(message);
#endif
    }
}

// Process incoming control mesages
void processStatus(ConnectionType connectionType) {
    // Control initial condition messages (recommended)
    String message = dashDevice.getButtonMessage(BUTTON_ID, toggle);
    message += dashDevice.getSingleBarMessage(SLIDER_ID, 25);
    int data[] = {100, 200, 300, 400, 500};
    dashDevice.addChartLineInts(message, CHART_ID, "L1", "Line 1", peakBar, "purple", yLeft, data, sizeof(data)/sizeof(int));
    sendMessage(connectionType, message);
    
    int data2[] = {100, 200, 300, 250, 225, 280, 350, 500, 550, 525, 500};
    dashDevice.addChartLineInts(message, CHART_ID, "L2", "Line 2", line, "aqua", yLeft, data2, sizeof(data2)/sizeof(int));
    sendMessage(connectionType, message);
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == BUTTON_ID) {
        if (toggle == off) {
            toggle = on;
            int data[] = {50, 255, 505, 758, 903, 400, 0};
            String message = "";
            dashDevice.addChartLineInts(message, CHART_ID, "L1", "Line 1 a", bar, "green", yLeft, data, sizeof(data)/sizeof(int));
            int data2[] = {153, 351, 806, 900, 200, 0};
            dashDevice.addChartLineInts(message, CHART_ID, "L2", "Line 2 a", segBar, "yellow", yLeft, data2, sizeof(data2)/sizeof(int));
            sendMessage(messageData->connectionType, message);
        } else if (toggle == on) {
            toggle = off;
            float data[] = {200, 303.334, 504.433, 809.434, 912, 706, 643};
            String message = "";
            dashDevice.addChartLineFloats(message, CHART_ID, "L1", "Line 1 b", peakBar, "orange", yLeft, data, sizeof(data)/sizeof(float));
            int data2[] = {};
            dashDevice.addChartLineInts(message, CHART_ID, "L2", "Line 2 b", peakBar, "blue", yLeft, data2, sizeof(data2)/sizeof(int));
            sendMessage(messageData->connectionType, message);
        }
        
        String message = dashDevice.getButtonMessage(BUTTON_ID, toggle);
        int data[] = {25, 75};
        message += dashDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]);
        sendMessage(messageData->connectionType, message);
    }
}

void processSlider(MessageData *messageData) {
    String payload = messageData->payloadStr;
    int data[2];
    data[0] = 100 - payload.toInt();
    data[1] = payload.toInt()/2;
    sendMessage(messageData->connectionType, dashDevice.getDoubleBarMessage(SLIDER_ID, data[0], data[1]));
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData->connectionType);
        break;
    case button:
        processButton(messageData);
        break;
    case slider:
        processSlider(messageData);
        break;
    default:
        dashProvision.processMessage(messageData);
        break;
    }
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (!bleActive) {
        if (commsChanged) {
            bleTimer = MIN_BLE_TIME_S; // Force reconnect of WiFi
            bleActive = true;
        } else {
#ifndef NO_MQTT
            mqtt_con.sendWhoAnnounce(); // Update announce topic with new name
#endif
        }
    }
}

// Timer Interrupt
static bool onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

void startBLE() {
    Serial.println("Startup BLE");
    bleActive = true;
    wifi.end();

    bleTimer = 0;
    ble_con.begin();
}

void startWiFi() {
    Serial.println("Startup WiFi");
    bleActive = false;
    ble_con.end();

#ifndef NO_MQTT
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword); // Setup MQTT host
#endif
    while (!wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword, 0)) {
        if (digitalRead(buttonPin) == HIGH) {
            break;
        }
    }
}

void setup() {  
    // Start the Serial port
    Serial.begin(115200);
    delay(2000);
  
    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashDevice.setup(wifi.macAddress());  // device type, unique deviceID, and device name
    Serial.print(F("Device ID: "));
    Serial.println(dashDevice.deviceID);

    ble_con.setCallback(&processIncomingMessage);
#ifndef NO_TCP
    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);
#endif

#ifndef NO_MQTT
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword); // Setup MQTT host
    wifi.attachConnection(&mqtt_con);
#endif

    startWiFi();

    timer.every(1000, onTimerCallback); // 1000ms
}

void loop() {
    timer.tick();

    if (bleActive) {
        ble_con.run();
        
        if (oneSecond) { // Tasks to occur every second
            oneSecond = false;
            Serial.print("BLE Timer: ");
            Serial.println(bleTimer);
            if ((bleTimer >= MIN_BLE_TIME_S) && (ble_con.isConnected() == false)) {
                startWiFi();
            }
            bleTimer += 1;
        }
    } else {
        int buttonState = digitalRead(buttonPin); // read the button pin    
        if (buttonState) {
            if (!ble_con.isConnected()) {
                startBLE();
            }
        }

        if (!bleActive && !wifi.run()) { // WiFi has dropped off
            Serial.println("WiFi disconnected");
            startWiFi();
        }
    }
}
