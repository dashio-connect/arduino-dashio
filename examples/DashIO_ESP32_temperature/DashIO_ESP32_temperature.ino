/*
 * Complete dash demonstration project for a temperature sensor attached to an ESP32.
 * Compatible with Dallas one-wore temperature sensors.
 * Communicates via BLE and MQTT connections.
 * Includes full configuration, graphical display and alarms (push notifications).
 * Min and max temperature alarm setpoints are stored in non-volatile memory.
 * Sensor attached to pin 13, but can be changed.
 * Uses provisioning to setup WiFi and MQTT username and password.
 * Connect with BLE first, then use provision to update the WiFi and MQTT connections.
 * Uses the serial monitor to show what is going on (115200 baud).
 * Requires the dash MQTT server for continuous temperature storage and push notifications.
 */

#include "DashioESP.h" // Dash ESP core library
#include "DashioProvisionESP.h" // Dash provisioning library to allow setup via BLE connection.
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

#define PREFS_NAME "dashio"
#define GRAPH_UPDATE_SECONDS (60 * 10) // Every 10 mins

const char configC64Str[] PROGMEM =
"zZbbbuM2EIZfJdC1GFCijrkjRdIJ1odUUbILLHqhWLQtRJYCWd44G+Sd+gx9sg4lOfFp0W6RFnsRgxwOx5xvZn7nxRiyoXHx9cV4"
"vq5WeZNXZZzCp3GBz13LNY2nPGsWvckyjSZvCrX1NC6MZHJtmMaqeS4U7Abx5FbvH9Nalc0VB1PEsAWW9TrPYEctFgnmWsjBPnxQ"
"R6LQ9yTChNsWhRMeCvCeVmVTV0UbAN7XRmi/GfajdHOWqOWjqtNmXaszWqT1sr1TVDWcx9VzWpyxYq3AuDlMyjQWKp8vmrckbcdy"
"7FfzVP6+G3x4/tjHXkgtJAiG/BmmKKS2RJxRK3AD148EP5G/vZt/Xn58/r+bxqfxhEEjwOpmyONulYgvSdsc/fM9SqPAJQxxj1Hk"
"iMBCjHOMCJEch44T2oIdPD/p8j9BpMybFWz//CPayW4nM+PorZZrOX54XITxZKy7plGbhhb5XJsiMU5EDMZZVS/TRjvdjmB7VGV4"
"Wq2m+aqNBLuH+yx5flTvUQ8a4ATRaVGt1Kf7bFLeqBIwNfVa6Y7qoRFCJfdDqHTAdM8HPqIRsxH27YA6xA8c7h9DIz8BTY9EZ/kf"
"gJ27vrcHjexBay/tMsPn3glq58T9W3DMcikPowBJLqDbbIFRSISPuIg86XPP9rh3DM7+ZcEFVvBfgoN55Vf94I7odb8Q49tuNZwM"
"ugW949s5j5LePxr2C35397md+K2ksUnMRXyjOTd1AcAk4L7Jv8OZAy+Z13kWVcV6WQJd2wYpXAD7ztK9yzTK9fLNxdqvV18dHZpV"
"dabq6JSKtccP8zL74en24PMib/ZuJHVartp2mD53w9p7siKdPuy0Qn8p0VtWbbYBJ3B/rs5ilR04HAXWJOLqCXIkZMf1HZeufg7J"
"j9NlW/CqVEZXNDrsq3AZJ73wDuLry13ldTwZ2ZFkiAjOkePhAAUcBoIz5mHqEyEZ1U1HN/kKfiHg2/A+6SueDN4c0g0Uyz01Jwdz"
"QDwSYvdH8twGG6b3qtifrOOBOR4GmLD2/4v3GHHTlaG1jNdLltYA031PSjvgfyLI2yT1BQtjrCmzJBnvAaXCiqRDkIiYg5wwkIgR"
"biEuLUYlkTaXWkm2Y3B5Nbgcwl8Ctmo227bHKK0ryHMfdadB9+umqUpRpveFyrajUJXbm4NaqfKkVB1KkRP62HkvQRfxZ1C3gq0l"
"q+O7rzG6DY5FRtd8p1eZKgpjR5lF4PkytB1EGRHICUIfBdh2kfDsgFPpuZKSfw3P+qXgtaL9AfDaFhz0qhxJ0OIXI1Pf8qm6Uc36"
"EfxKcDef8lluZulqYbKhMEe/JZoZCChvXe9y9dSL6Gweq28wxa+vfwE=";

// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 2);

// Create Connections
DashBLE  ble_con(&dashDevice, true);
DashMQTT mqtt_con(&dashDevice, true, true);
DashWiFi wifi;
DashProvision dashProvision(&dashDevice);

// Create Control IDs
const char *TEMPTB_ID = "TB01";
const char *ALARMTB_LOW_ID = "TB02";
const char *ALARMTB_HIGH_ID = "TB03";
const char *AEB_LOW_ID = "B01";
const char *AEB_HIGH_ID = "B02";
const char *GRAPH_ID = "IDTG";

OneWire oneWire(13); // Temperature sensor connected to pin 13
DallasTemperature tempSensor(&oneWire);

int graphSecondsCounter = 0;
float temperatureC;
float tempSum = 0;

bool alarmEnableLow = on;
bool alarmEnableHigh = on;
float minTemp = 0;
float maxTemp = 100;

bool oneSecond = false;
float sampleTempC;

void sendMessage(ConnectionType connectionType, String message) {
    if (connectionType == BLE_CONN) {
        ble_con.sendMessage(message);
    } else if (connectionType == MQTT_CONN) {
        mqtt_con.sendMessage(message);
    }
}

void sendMessageAll(String message) {
    mqtt_con.sendMessage(message);
    ble_con.sendMessage(message);
}

void onProvisionCallback(ConnectionType connectionType, const String& message, bool commsChanged) {
    sendMessage(connectionType, message);

    if (commsChanged) {
        mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
        wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);
    }
}

String getButtonMessages() {
    String message = "";
    if (alarmEnableLow) {
        message += dashDevice.getButtonMessage(AEB_LOW_ID, alarmEnableLow, "bell");
    } else {
        message += dashDevice.getButtonMessage(AEB_LOW_ID, alarmEnableLow, "bell", "OFF");
    }

    if (alarmEnableHigh) {
        message += dashDevice.getButtonMessage(AEB_HIGH_ID, alarmEnableHigh, "bell");
    } else {
        message += dashDevice.getButtonMessage(AEB_HIGH_ID, alarmEnableHigh, "bell", "OFF");
    }
    return message;
}

void processStatus(MessageData *messageData) {
    String message = getButtonMessages();

    message += dashDevice.getTextBoxMessage(ALARMTB_LOW_ID, String(minTemp));
    message += dashDevice.getTextBoxMessage(ALARMTB_HIGH_ID, String(maxTemp));

    message += dashDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

    message += dashDevice.getTimeGraphLine(GRAPH_ID, "L1", "Avge Temp", line, "red", yLeft);

    sendMessage(messageData->connectionType, message);
}

void processButton(MessageData *messageData) {
    dashProvision.preferences.begin(PREFS_NAME, false);
    if (messageData->idStr == AEB_LOW_ID) {
        if (alarmEnableLow == on) {
            alarmEnableLow = off;
        } else {
            alarmEnableLow = on;
        }
        dashProvision.preferences.putBool("AlmEnLow", alarmEnableLow);
    } else if (messageData->idStr == AEB_HIGH_ID) {
        if (alarmEnableHigh == on) {
            alarmEnableHigh = off;
        } else {
            alarmEnableHigh = on;
        }
        dashProvision.preferences.putBool("AlmEnHigh", alarmEnableHigh);
    }
    dashProvision.preferences.end();
}

void processTextBox(MessageData *messageData) {
    dashProvision.preferences.begin(PREFS_NAME, false);
    if (messageData->idStr == ALARMTB_LOW_ID) {
        minTemp = (messageData->payloadStr).toFloat();
        dashProvision.preferences.putFloat("MinTemp", minTemp);
    } else if (messageData->idStr == ALARMTB_HIGH_ID) {
        maxTemp = (messageData->payloadStr).toFloat();
        dashProvision.preferences.putFloat("MaxTemp", maxTemp);
    }
    dashProvision.preferences.end();
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData);
        break;
    case textBox:
        processTextBox(messageData);
        break;
    case button:
        processButton(messageData);
        break;
    default:
        dashProvision.processMessage(messageData);
        break;
    }
}

void setTemperatureEverySecond(float temperature) {
    String messageToSend = "";
    
    if (temperature > -100) {
        temperatureC = temperature;
        messageToSend = dashDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

        // Now calculate the average temperature and send every 10 minutes. Dash server will store this.
        tempSum += temperatureC;
        graphSecondsCounter++;
        if (graphSecondsCounter >= GRAPH_UPDATE_SECONDS) {
            float tempFiltered = tempSum / GRAPH_UPDATE_SECONDS;
            messageToSend += dashDevice.getTimeGraphPoint(GRAPH_ID, "L1", tempFiltered);

            String text = "The temperature is " + String(tempFiltered) + "°C";
            if (alarmEnableLow == on) {
                if (tempFiltered < minTemp) {
                    Notification tempAlarm = {"AL01", "Under Temp", text};
                    mqtt_con.sendMessage(dashDevice.getAlarmMessage(tempAlarm), alarm_topic);
                }
            }
            if (alarmEnableHigh == on) {
                if (tempFiltered > maxTemp) {
                    Notification tempAlarm = {"AL02", "Over Temp", text};
                    mqtt_con.sendMessage(dashDevice.getAlarmMessage(tempAlarm), alarm_topic);
                }
            }

            graphSecondsCounter = 0;
            tempSum = 0;
        }
    } else {
        messageToSend = dashDevice.getTextBoxMessage(TEMPTB_ID, "Err");
    }

    messageToSend += getButtonMessages();
    messageToSend += dashDevice.getTextBoxMessage(ALARMTB_LOW_ID, String(minTemp));
    messageToSend += dashDevice.getTextBoxMessage(ALARMTB_HIGH_ID, String(maxTemp));
    sendMessageAll(messageToSend);
}

void oneSecondTimerTask(void *parameters) {
    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        oneSecond = true;
    }
}

void generalSetup() {
    dashProvision.preferences.begin(PREFS_NAME, true);
    alarmEnableLow = dashProvision.preferences.getUChar("AlmEnLow", false);
    if (alarmEnableLow) {
        Serial.println("Alarm Low Enabled");
    } else {
        Serial.println("Alarm Low Disabed");
    }
    
    minTemp = dashProvision.preferences.getFloat("MinTemp", 10);
    Serial.println("Min Temp: " + String(minTemp));

    alarmEnableHigh = dashProvision.preferences.getUChar("AlmEnHigh", false);
    if (alarmEnableHigh) {
        Serial.println("Alarm High Enabled");
    } else {
        Serial.println("Alarm High Disabed");
    }

    maxTemp = dashProvision.preferences.getFloat("MaxTemp", 20);
    Serial.println("Max Temp: " + String(maxTemp));

    dashProvision.preferences.end();
}

void setup() {
    Serial.begin(115200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashDevice.setup(wifi.macAddress()); // unique deviceID
    
    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin();
    
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
    mqtt_con.setCallback(&processIncomingMessage);
    mqtt_con.addDashStore(timeGraph, GRAPH_ID);
        
    wifi.attachConnection(&mqtt_con);
    wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);

    generalSetup();

    // Setup 1 second timer task for measuring the temperature (overkill)
    xTaskCreate(
        oneSecondTimerTask,
        "One Second",
        1024, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );

    tempSensor.setWaitForConversion(false);
    tempSensor.begin();
    tempSensor.requestTemperaturesByIndex(0);
}

void loop() {
    ble_con.run();
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
