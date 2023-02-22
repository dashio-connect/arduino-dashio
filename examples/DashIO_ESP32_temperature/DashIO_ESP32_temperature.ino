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
"zZbbbqNIEIZfxeKajoDmmLtuutsTDbazmGRGWu0FNm27NRgijCfORnmnfYZ9sikOTnwaRSNlV3OD+lAUVV9V/fazFtFIu/7zWXu6"
"LTeqVmURp/DUro0rx3R07VFl9ao/MnWtVnUu95batZZMbjVd29RPuYTdMJ7cNfuHtJJFfcPgKKSGCSfbrcpgR0wacuqYyDY8eBBb"
"oMBzBTIws0wCNyzgYD0vi7oq89YBxNd6aL8M+1G6GyRy/SCrtN5WckDytFq37+RlBfdfVqqWsN+d5qNrK6mWq/o1P8s2betFv5S6"
"5/gfnrrhGW5ATMSxAalTg6CAWAIxSkzf8R0v5OxC6tZh6qr40NT/0rXP4wmF8sNqGrG4WyX8a9K2RB+5S0joO5gi5lKCbO6biDJm"
"IIwFMwLbDixOTyJPutQvwChUvYHtv/+EB4kdJKWdxWo6pu0F5/zHk3HTK7Xc1SRXy+Yo5OOEx3C4KKt1WjdGdyPYnhUYQqvkXG1a"
"T7D7NsuSpwf55vWk9heIzvNyIz/PskkxlQVgqqutbJqph4YxEcwLoMg+bTrd9xAJqYUMz/KJjT3fZt45NPwL0JpB6E7+B2BXjuce"
"QcNH0NqXDpkZV+4FalfYeRccNR3CgtBHgnHoNosbKMDcQ4yHrvCYa7nMPQdn/bbgfNP/L8HBvLKbfnBH5LZf8PFdt4omw25B7tl+"
"zsOktw+jfsHu77+0E79XMzqJGY+nDee6ygGYANxT9Tfc2RDJslJZWObbdQF0LQtUcAXsu5MuLl0rtutXE/O4Xn11Gte0rDJZhScC"
"1t58WxbZ/oLm2/35T22TKi02bQ/Mn7oJ7S1HKiuaOg96LxdUZxCVy95X0tzScrf/zgTcLuUgltmJwdn3Gipx+Qj5Ynxg+oau6QQF"
"IMbpui1+WUitKyCJ+op8ipNehIfx7adDFbZdEVqhoAhzxpDtGj7yGQwHo9Q1iIe5oKRpQLJTG/ih0K6dY+g3LBm+3qc7CNK4NDIn"
"I4FdHBjOz5S6dRalM5kfD9n57JzPBQxb+wej9THermlabdqg35PehhhNkvERHMLNUNgY8ZDayA58gShmJmLCpERgYTHRKES5WOzL"
"2tfzAFAnIrNtXZcFL9JZLrN9L5fF/rVIreVFqTmVEjvwDPuNW+fwV/i0gttIDhidaYTlXBKJplAH/UVlnmsHysp91xOBZSNCMUe2"
"H3jINywHcdfyGRGuIwh+H5L5O0FqxfUDILUtNezVMxSgmc9aJr+ruZzKevsAdgWY649qofQs3ax0GnF99EeSaK3Qsdb0XsnHXuwW"
"y1h+h+XLyw8=";

// Create device
DashDevice dashDevice(DEVICE_TYPE, configC64Str, 1);

// Create Connections
DashBLE  ble_con(&dashDevice, true);
DashMQTT mqtt_con(&dashDevice, true, true);
DashWiFi wifi;
DashProvision dashProvision(&dashDevice);

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

    message += dashDevice.getTimeGraphLine(GRAPH_ID, "L1", "Avge Temp", line, "red");

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
    if (temperature > -100) {
        temperatureC = temperature;
        messageToSend = dashDevice.getTextBoxMessage(TEMPTB_ID, String(temperatureC));

        tempSum += temperatureC;
        graphSecondsCounter++;
        if (graphSecondsCounter >= GRAPH_UPDATE_SECONDS) {
            float tempFiltered = tempSum / GRAPH_UPDATE_SECONDS;
            messageToSend += dashDevice.getTimeGraphPoint(GRAPH_ID, "L1", tempFiltered);

            String text = "The temperature is " + String(tempFiltered) + "°C";
            if (alarmEnableLow == on) {
                if (tempFiltered < minTemp) {
                    Notification tempAlarm = {"AL01", "Under Temp", text};
                    alarmMessageToSend = dashDevice.getAlarmMessage(tempAlarm);
                }
            }
            if (alarmEnableHigh == on) {
                if (tempFiltered > maxTemp) {
                    Notification tempAlarm = {"AL02", "Over Temp", text};
                    alarmMessageToSend = dashDevice.getAlarmMessage(tempAlarm);
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
    messageToSend.reserve(1024);

    timer = timerBegin(0, 80, true); // Use 1st timer of 4. 1 tick takes 1/(80MHZ/80) = 1us so we set divider 80 and count up
    timerAttachInterrupt(timer, &onTimer, true); //Attach onTimer function to our timer
    timerAlarmWrite(timer, 1000000, true); // 1 tick is 1us => 1s.
    timerAlarmEnable(timer);
        
    Serial.begin(115200);

    DeviceData defaultDeviceData = {DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, MQTT_USER, MQTT_PASSWORD};
    dashProvision.load(&defaultDeviceData, &onProvisionCallback);

    dashDevice.setup(wifi.macAddress()); // unique deviceID
    
    ble_con.setCallback(&processIncomingMessage);
    ble_con.begin();
    
    mqtt_con.setup(dashProvision.dashUserName, dashProvision.dashPassword);
    mqtt_con.setCallback(&processIncomingMessage);
    
    wifi.attachConnection(&mqtt_con);
    wifi.begin(dashProvision.wifiSSID, dashProvision.wifiPassword);

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
