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
#define EEPROM_START PROVISIONING_EEPROM_SIZE
#define ADDITIONAL_EEPROM_SIZE 10

// Create device
DashioDevice dashioDevice(DEVICE_TYPE);
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

void processConfig1(MessageData *messageData) {
    String message = dashioDevice.getConfigMessage(DeviceCfg(1, "name, wifi, dashio"));  // One device view

    TextBoxCfg tempTextBox(TEMPTB_ID, CB01_ID, "Temperature", {0, 0, 1, 0.1515});
    tempTextBox.titlePosition = titleOff;
    tempTextBox.format = numFmt;
    tempTextBox.kbdType = noKbd;
    tempTextBox.units = "°C";
    message += dashioDevice.getConfigMessage(tempTextBox);

    TimeGraphCfg tempGraph(GRAPH_ID, CB01_ID, "Temperature", {0, 0.1515, 1, 0.3636});
    tempGraph.titlePosition = titleOff;
    tempGraph.yAxisLabel = "°C";
    tempGraph.yAxisMin = 0;
    tempGraph.yAxisMax = 40;
    tempGraph.yAxisNumBars = 5;
    message += dashioDevice.getConfigMessage(tempGraph);

    LabelCfg labelHigh(LABEL_HIGH_ID, CB01_ID, "Max Temperature Alarm", {0, 0.5151, 1, 0.2424});
    message += dashioDevice.getConfigMessage(labelHigh);

    ButtonCfg alarmEnableHighButton(AEB_HIGH_ID, CB01_ID, "Enable", {0.05, 0.5758, 0.25, 0.15});
    alarmEnableHighButton.titlePosition = titleOff;
    alarmEnableHighButton.iconName = "bell";
    alarmEnableHighButton.offColor = "red";
    alarmEnableHighButton.onColor = "lime";
    message += dashioDevice.getConfigMessage(alarmEnableHighButton);

    TextBoxCfg alarmMaxTempTextBox(ALARMTB_HIGH_ID, CB01_ID, "Max °C", {0.35, 0.5758, 0.6, 0.1515});
    alarmMaxTempTextBox.titlePosition = titleOff;
    alarmMaxTempTextBox.format = numFmt;
    alarmMaxTempTextBox.precision = 3;
    alarmMaxTempTextBox.kbdType = numKbd;
    alarmMaxTempTextBox.units = "°C";
    message += dashioDevice.getConfigMessage(alarmMaxTempTextBox);
    sendMessage(messageData->connectionType, message);
}

void processConfig2(MessageData *messageData) {
    LabelCfg labelLow(LABEL_LOW_ID, CB01_ID, "Min Temperature Alarm", {0, 0.7576, 1, 0.2424});
    String message = dashioDevice.getConfigMessage(labelLow);

    ButtonCfg alarmEnableLowButton(AEB_LOW_ID, CB01_ID, "Enable", {0.05, 0.8181, 0.25, 0.15});
    alarmEnableLowButton.titlePosition = titleOff;
    alarmEnableLowButton.iconName = "bell";
    alarmEnableLowButton.offColor = "red";
    alarmEnableLowButton.onColor = "lime";
    message += dashioDevice.getConfigMessage(alarmEnableLowButton);

    TextBoxCfg alarmMinTempTextBox(ALARMTB_LOW_ID, CB01_ID, "Max °C", {0.35, 0.8181, 0.6, 0.1515});
    alarmMinTempTextBox.titlePosition = titleOff;
    alarmMinTempTextBox.format = numFmt;
    alarmMinTempTextBox.precision = 3;
    alarmMinTempTextBox.kbdType = numKbd;
    alarmMinTempTextBox.units = "°C";
    message += dashioDevice.getConfigMessage(alarmMinTempTextBox);
    
    // Connections
    BLEConnCfg bleCnctnConfig(SERVICE_UUID, CHARACTERISTIC_UUID, CHARACTERISTIC_UUID);
    message += dashioDevice.getConfigMessage(bleCnctnConfig);
  
    MQTTConnCfg mqttCnctnConfig(dashioProvision.dashUserName, DASH_SERVER);
    message += dashioDevice.getConfigMessage(mqttCnctnConfig);

    AlarmCfg alarmHighCfg("AL02", "Over Temperature", "boing");
    message += dashioDevice.getConfigMessage(alarmHighCfg);

    AlarmCfg alarmLowCfg("AL01", "Under Temperature", "boing");
    message += dashioDevice.getConfigMessage(alarmLowCfg);

    // Device View
    DeviceViewCfg deviceView(CB01_ID, "Temperature Log", "Termperature", "16");
    deviceView.ctrlMaxFontSize = 45;
    deviceView.color = 16;
    deviceView.ctrlColor = "white";
    deviceView.ctrlBorderColor = "white";
    deviceView.ctrlBkgndColor = "blue";
    deviceView.ctrlTitleBoxColor = 5;
    message += dashioDevice.getConfigMessage(deviceView);
    sendMessage(messageData->connectionType, message);
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == AEB_LOW_ID) {
        if (alarmEnableLow == on) {
            alarmEnableLow = off;
            EEPROM.write(EEPROM_START, 0);
        } else {
            alarmEnableLow = on;
            EEPROM.write(EEPROM_START, 1);
        }
        EEPROM.commit();
    } else if (messageData->idStr == AEB_HIGH_ID) {
        if (alarmEnableHigh == on) {
            alarmEnableHigh = off;
            EEPROM.write(EEPROM_START + 5, 0);
        } else {
            alarmEnableHigh = on;
            EEPROM.write(EEPROM_START + 5, 1);
        }
        EEPROM.commit();
    }
}

void processTextBox(MessageData *messageData) {
    if (messageData->idStr == ALARMTB_LOW_ID) {
        minTemp = (messageData->payloadStr).toFloat();
        EEPROM.put(EEPROM_START + 1, minTemp);
        EEPROM.commit();
    } else if (messageData->idStr == ALARMTB_HIGH_ID) {
        maxTemp = (messageData->payloadStr).toFloat();
        EEPROM.put(EEPROM_START + 5 + 1, maxTemp);
        EEPROM.commit();
    }
}

void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData);
        break;
    case config:
        processConfig1(messageData);
        processConfig2(messageData);
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
    if (EEPROM.read(EEPROM_START) > 0) {
        alarmEnableLow = on;
        Serial.println("Alarm Low Enabled");
    } else {
        alarmEnableLow = off;
        Serial.println("Alarm Low Disabed");
    }

    float temp;
    EEPROM.get(EEPROM_START + 1, temp);
    if (!isnan(temp)) {
        minTemp = temp;
        Serial.println("Min Temp: " + String(minTemp));
    }
    
    if (EEPROM.read(EEPROM_START + 5) > 0) {
        alarmEnableHigh = on;
        Serial.println("Alarm High Enabled");
    } else {
        alarmEnableHigh = off;
        Serial.println("Alarm High Disabed");
    }

    EEPROM.get(EEPROM_START + 5 + 1, temp);
    if (!isnan(temp)) {
        maxTemp = temp;
        Serial.println("Max Temp: " + String(maxTemp));
    }
}

void setup() {
    messageToSend.reserve(1024);

    timer = timerBegin(0, 80, true); // Use 1st timer of 4. 1 tick takes 1/(80MHZ/80) = 1us so we set divider 80 and count up
    timerAttachInterrupt(timer, &onTimer, true); //Attach onTimer function to our timer
    timerAlarmWrite(timer, 1000000, true); // 1 tick is 1us => 1s.
    timerAlarmEnable(timer);
        
    Serial.begin(115200);

    dashioProvision.setup(ADDITIONAL_EEPROM_SIZE);
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
