/*
 * Complete DashIO demonstration project for a temperature sensor attached to an ESP32.
 * Compatible with Dallas one-wore temperature sensors.
 * Communicates via BLE and MQTT connections.
 * Includes full configuration, graphical display and alarms (push notifications).
 * Min and max temperature alarm setpoints are stored in non-volatile memory.
 * Sensor attached to pin 13, but can be changed.
 * Uses provisioning to setup WiFi and MQTT username and password.
 * Connect with BLE first, then use provision to update the WiFi and MQTT connections.
 * Uses the serial monitor to show what is going on (115200 baud).
 * Requires the DashIO MQTT server for continuous temperature storage and push notifications.
 */

#include "DashioESP.h" // DashIO ESP core library
#include "DashioProvisionESP.h" // DashIO provisioning library to allow setup via BLE connection.
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
"zVbNctowEH4VRmdgMLYJ4YYdk8mUQIdAeuj0IPACmsoSleWUNJN36jP0ybqy5cZgmKZ/M714vKvV7n7f7kp6IgmILCWD9x+ahNMl"
"cPP/RB7fypRpJsWM4pcMOm3f8ZvkM4v11qqcJtFMcygtyYDMp29Jk6T6kQNK17Ppwsi5Fcq3dN+YQ7IDRXWmoDHkVCW4vqMKhL65"
"QpMw6DioWUmhleS5ahyMrY5LhfK7LdOA8v44wybZAtts9Y+Mu173uXkKyoXf/1MoTPwmlO5vQ8ECadjrQO6hqFHV9byIVo+fCabR"
"nHz7Glbyr+ROasEcv1vnYzKdRKTIYMjZxqjCaDKPZqhcS5VQbYwWtyjWCMe8FKxYmntC6eMynj/u4MXrUS1O8LHiMoU3y3gq7kDE"
"ZKBVBqa4Rxy4v8CBacdC86/xt/2L3gEH7gEH+aYqBZ127wQJbdd/NQ/d/5OHvtP/lzzghCR0Zw+zZaa1FNdKZoeaYnbkeh3aKZxB"
"fDSnBX+FeSTokoON0CRSlNvGLIGTLNdYfKG6cEbOsnqmcwzZaFRjp+ufoqeDWoZwJjQxMQPgnJgW+Qlk53+BnDfJX4CMFd8outuW"
"1ecQSiFgVcR/IgpovFgU6KPA9/pu1HJ7kdPyev1+K7gY+a1o2Llwe07Q7Q37mEoK6oGtwG7yRsNR2O04LWcU+C3Pv4xa/VEYtkI/"
"vAxd17l0vMDkr/CQ/5U4zzgUQi5t2sknrQ/z3spUL2ZjdBfTdNs2HybbSAQOOGZoOTBuYka5dWOunExZgZrbyv5rrOl1SRNelsM9"
"S/F2Kw7dSoPcXM2vTbnydbonA6/zik5we965Syf3NDYvjsMT6TUtkp9MuYNJlgTU4Ko/TmptYjoiBlNBJHTNNoZMkSVXueqewWd0"
"4zyXNlZh7lmtOEIeIRt37Asi8TBYukXsOBhZIspBQV+FIs3jV9krHwPoKfi4EXE5UQHPoNRLFYOa/vD2oguPngtmpaaz8i2LhalA"
"w3o+wX1jLDfVXOaKijSv5OqxKDuuzM0+fGqUcaZotIGGPTgqBud3v/BlBroynhMpIB/PFDi2tSzbMuUM0VohZqpoeSvDA3YaZl62"
"cBYzec/SzHb483c=";

// Create device
DashioDevice dashioDevice(DEVICE_TYPE, configC64Str);
// Create Connections
DashioBLE  ble_con(&dashioDevice, true);
DashioMQTT mqtt_con(&dashioDevice, 2048, true, true);
DashioWiFi wifi;
DashioProvision dashioProvision(&dashioDevice);

// Create Control IDs
const char *CB01_ID = "CB01";
const char *TEMPTB_ID = "TB01";
const char *ALARMTB_LOW_ID = "TB02";
const char *ALARMTB_HIGH_ID = "TB03";
const char *AEB_LOW_ID = "B01";
const char *AEB_HIGH_ID = "B02";
const char *GRAPH_ID = "IDTG";
const char *LABEL_HIGH_ID = "LBL01";
const char *LABEL_LOW_ID = "LBL02";

hw_timer_t * timer = NULL; // Hardware timer

OneWire oneWire(13); // Temperature sensor connected to pin 13
DallasTemperature tempSensor(&oneWire);

String messageToSend((char *)0);
String alarmMessageToSend = "";
int graphSecondsCounter = 0;
float temperatureC;
float tempSum = 0;

bool alarmEnableLow = on;
bool alarmEnableHigh = on;
float minTemp = 0;
float maxTemp = 100;

bool oneSecond = false;
float sampleTempC;

void IRAM_ATTR onTimer() {
    oneSecond = true;
}

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
        mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
        wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);
    }
}

String getButtonMessages() {
    String message = "";
    if (alarmEnableLow) {
        message += dashioDevice.getButtonMessage(AEB_LOW_ID, alarmEnableLow, "bell");
    } else {
        message += dashioDevice.getButtonMessage(AEB_LOW_ID, alarmEnableLow, "bell", "OFF");
    }

    if (alarmEnableHigh) {
        message += dashioDevice.getButtonMessage(AEB_HIGH_ID, alarmEnableHigh, "bell");
    } else {
        message += dashioDevice.getButtonMessage(AEB_HIGH_ID, alarmEnableHigh, "bell", "OFF");
    }
    return message;
}

void processStatus(MessageData *messageData) {
    String message = getButtonMessages();

    message += dashioDevice.getTextBoxMessage(ALARMTB_LOW_ID, String(minTemp));
    message += dashioDevice.getTextBoxMessage(ALARMTB_HIGH_ID, String(maxTemp));

    message += dashioDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

    message += dashioDevice.getTimeGraphLine(GRAPH_ID, "L1", "Avge Temp", line, "red");

    sendMessage(messageData->connectionType, message);
}

void processButton(MessageData *messageData) {
    dashioProvision.preferences.begin("dashio", false);
    if (messageData->idStr == AEB_LOW_ID) {
        if (alarmEnableLow == on) {
            alarmEnableLow = off;
        } else {
            alarmEnableLow = on;
        }
        dashioProvision.preferences.putBool("AlmEnLow", alarmEnableLow);
    } else if (messageData->idStr == AEB_HIGH_ID) {
        if (alarmEnableHigh == on) {
            alarmEnableHigh = off;
        } else {
            alarmEnableHigh = on;
        }
        dashioProvision.preferences.putBool("AlmEnHigh", alarmEnableHigh);
    }
    dashioProvision.preferences.end();
}

void processTextBox(MessageData *messageData) {
    dashioProvision.preferences.begin("dashio", false);
    if (messageData->idStr == ALARMTB_LOW_ID) {
        minTemp = (messageData->payloadStr).toFloat();
        dashioProvision.preferences.putFloat("MinTemp", minTemp);
    } else if (messageData->idStr == ALARMTB_HIGH_ID) {
        maxTemp = (messageData->payloadStr).toFloat();
        dashioProvision.preferences.putFloat("MaxTemp", maxTemp);
    }
    dashioProvision.preferences.end();
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
        dashioProvision.processMessage(messageData);
        break;
    }
}

void setTemperatureEverySecond(float temperature) {
    if (temperature > -100) {
        temperatureC = temperature;
        messageToSend = dashioDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

        tempSum += temperatureC;
        graphSecondsCounter++;
        if (graphSecondsCounter >= GRAPH_UPDATE_SECONDS) {
            float tempFiltered = tempSum / GRAPH_UPDATE_SECONDS;
            messageToSend += dashioDevice.getTimeGraphPoint(GRAPH_ID, "L1", tempFiltered);

            String text = "The temperature is " + String(tempFiltered) + "°C";
            if (alarmEnableLow == on) {
                if (tempFiltered < minTemp) {
                    Notification tempAlarm = {"AL01", "Under Temp", text};
                    alarmMessageToSend = dashioDevice.getAlarmMessage(tempAlarm);
                }
            }
            if (alarmEnableHigh == on) {
                if (tempFiltered > maxTemp) {
                    Notification tempAlarm = {"AL02", "Over Temp", text};
                    alarmMessageToSend = dashioDevice.getAlarmMessage(tempAlarm);
                }
            }

            graphSecondsCounter = 0;
            tempSum = 0;
        }
    } else {
        messageToSend = dashioDevice.getTextBoxMessage(TEMPTB_ID, "Err");
    }

    messageToSend += getButtonMessages();
    messageToSend += dashioDevice.getTextBoxMessage(ALARMTB_LOW_ID, String(minTemp));
    messageToSend += dashioDevice.getTextBoxMessage(ALARMTB_HIGH_ID, String(maxTemp));
}

void generalSetup() {
    dashioProvision.preferences.begin("dashio", true);
    alarmEnableLow = dashioProvision.preferences.getUChar("AlmEnLow", false);
    if (alarmEnableLow) {
        Serial.println("Alarm Low Enabled");
    } else {
        Serial.println("Alarm Low Disabed");
    }
    
    minTemp = dashioProvision.preferences.getFloat("MinTemp", 10);
    Serial.println("Min Temp: " + String(minTemp));

    alarmEnableHigh = dashioProvision.preferences.getUChar("AlmEnHigh", false);
    if (alarmEnableHigh) {
        Serial.println("Alarm High Enabled");
    } else {
        Serial.println("Alarm High Disabed");
    }

    maxTemp = dashioProvision.preferences.getFloat("MaxTemp", 20);
    Serial.println("Max Temp: " + String(maxTemp));

    dashioProvision.preferences.end();
}

void setup() {
    messageToSend.reserve(1024);

    timer = timerBegin(0, 80, true); // Use 1st timer of 4. 1 tick takes 1/(80MHZ/80) = 1us so we set divider 80 and count up
    timerAttachInterrupt(timer, &onTimer, true); //Attach onTimer function to our timer
    timerAlarmWrite(timer, 1000000, true); // 1 tick is 1us => 1s.
    timerAlarmEnable(timer);
        
    Serial.begin(115200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashioProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashioDevice.setup(wifi.macAddress()); // unique deviceID
    
    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin();
    
    mqtt_con.setup(dashioProvision.dashUserName, dashioProvision.dashPassword);
    mqtt_con.setCallback(&processIncomingMessage);
    
    wifi.attachConnection(&mqtt_con);
    wifi.begin(dashioProvision.wifiSSID, dashioProvision.wifiPassword);

    generalSetup();

    tempSensor.setWaitForConversion(false);
    tempSensor.begin();
    tempSensor.requestTemperaturesByIndex(0);
}

void loop() {
    if (messageToSend.length() > 0) {
        sendMessageAll(messageToSend);
        messageToSend = "";
    }
    
    if (alarmMessageToSend.length() > 0) {
        mqtt_con.sendMessage(alarmMessageToSend, alarm_topic);
        alarmMessageToSend = "";
    }

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
