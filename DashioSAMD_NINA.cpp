/*
 MIT License

 Copyright (c) 2021 Craig Tuffnell, DashIO Connect Limited

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010 || defined ARDUINO_SAMD_MKRVIDOR4000
//#ifdef ARDUINO_ARCH_SAMD

#include "DashioSAMD_NINA.h"
#include <MDNS_Generic.h>

#define INCOMING_BUFFER_SIZE 512

// WiFi
const int WIFI_CONNECT_TIMEOUT_MS = 5000; // 5s

// MQTT
const uint8_t MQTT_QOS     = 2;
const int     MQTT_RETRY_S = 10; // Retry after 10 seconds

// BLE
const int BLE_MAX_SEND_MESSAGE_LENGTH = 100;

// mDNS
WiFiUDP udp;
MDNS mdns(udp);

// ---------------------------------------- WiFi ---------------------------------------

bool DashioWiFi::oneSecond = false;

// Timer Interrupt
bool DashioWiFi::onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

bool DashioWiFi::begin(char *ssid, char *password, int maxRetries) {
    wiFiDrv.wifiDriverDeinit(); // Required when switching from BLE to WiFi
    wiFiDrv.wifiDriverInit();

    // Check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println(F("WiFi module failed!"));
        return false;
    }

    WiFi.setTimeout(WIFI_CONNECT_TIMEOUT_MS); // 10s

    // Connect to Wifi Access Point
    status = WL_IDLE_STATUS;
    int retryCount = 0;
    while (status != WL_CONNECTED) {
        if (retryCount > maxRetries) {
            return false;
        }
        retryCount += 1;

        Serial.print(F("Attempting to connect to SSID: "));
        Serial.print(ssid);
    
        // Connect to WPA/WPA2 network
        WiFi.disconnect();
        delay(1000);
        status = WiFi.begin(ssid, password);
        
        Serial.print(" Status: "); // 1 = no SSID avail = incorrect password, 4 = connection fail, 3 = connected
        Serial.println(status);
    }

    // Device IP address
    ipAddr = WiFi.localIP();
    Serial.print(F("IP Address: "));
    Serial.println(ipAddr);
    
    if (tcpConnection != NULL) {
        tcpConnection->begin();
    }

    timer.every(1000, onTimerCallback); // 1000ms

    return true;
}

void DashioWiFi::attachConnection(DashioTCP *_tcpConnection) {
    tcpConnection = _tcpConnection;
}

void DashioWiFi::attachConnection(DashioMQTT *_mqttConnection) {
    mqttConnection = _mqttConnection;
}

bool DashioWiFi::run() {
    timer.tick();
    
    if (WiFi.status() == WL_CONNECTED) {
        if (tcpConnection != NULL) {
            tcpConnection->run();
        }

        if (mqttConnection != NULL) {
            if (oneSecond) {
                oneSecond = false;
                mqttConnection->checkConnection();
            }
            mqttConnection->run();
        }
    } else {
        return false;
    }
    
    return true;
}

void DashioWiFi::end() {
    if (tcpConnection != NULL) {
        tcpConnection->end();
    }

    if (mqttConnection != NULL) {
        mqttConnection->end();
    }

    WiFi.disconnect();
    WiFi.end();
    wiFiDrv.wifiDriverDeinit();
    status = WL_DISCONNECTED;
}

byte * DashioWiFi::macAddress() {
    static byte mac[6];
    WiFi.macAddress(mac);
    return mac;
}

String DashioWiFi::ipAddress() {
    static char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
    return ipStr;
}

// ---------------------------------------- TCP ----------------------------------------

DashioTCP::DashioTCP(DashioDevice *_dashioDevice, bool _printMessages, uint16_t _tcpPort) : wifiServer(_tcpPort), mdns(udp), messageData(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
}

void DashioTCP::setCallback(void (*processIncomingMessage)(MessageData *connection)) {
    processTCPmessageCallback = processIncomingMessage;
}

void DashioTCP::sendMessage(const String& message) {
    if (client.connected()) {
        client.print(message);

        if (printMessages) {
            Serial.println(F("---- TCP Sent ----"));
            Serial.println(message);
        }
    }
}

void DashioTCP::begin() {
    wifiServer.begin();

    mdns.begin(WiFi.localIP(), dashioDevice->deviceID.c_str());
    String service = "_";
    service += dashioDevice->deviceID;
    service += "._DashIO";
    if (printMessages) {
        Serial.println("Starting mDNS " + service);
    }
    mdns.addServiceRecord(service.c_str(), tcpPort, MDNSServiceTCP);
}

void DashioTCP::run() {
    mdns.run();

    if (!client) {
        client = wifiServer.available();
        client.setTimeout(2000);
    } else {
       if (client.connected()) {
            while (client.available()>0) {
                char c = client.read();
                if (messageData.processChar(c)) {
                    if (printMessages) {
                        Serial.println(messageData.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(messageData.control)));
                    }
      
                    switch (messageData.control) {
                    case who:
                        sendMessage(dashioDevice->getWhoMessage());
                        break;
                    case connect:
                        sendMessage(dashioDevice->getConnectMessage());
                        break;
                    case config:
                        dashioDevice->dashboardID = messageData.idStr;
                        if (dashioDevice->configC64Str != NULL) {
                            sendMessage(dashioDevice->getC64ConfigMessage());
                        } else {
                            if (processTCPmessageCallback != NULL) {
                                processTCPmessageCallback(&messageData);
                            }
                        }
                        break;
                    default:
                        if (processTCPmessageCallback != NULL) {
                            processTCPmessageCallback(&messageData);
                        }
                        break;
                    }
                }
            }
        } else {
            client.stop();
            client = wifiServer.available();
            client.setTimeout(2000);
        }
    }
}

void DashioTCP::end() {
    client.stop();
}

// ---------------------------------------- MQTT ---------------------------------------

MessageData DashioMQTT::messageData(MQTT_CONN, INCOMING_BUFFER_SIZE);
WiFiSSLClient DashioMQTT::wifiClient;
MqttClient DashioMQTT::mqttClient(wifiClient);

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
}

void DashioMQTT::messageReceivedMQTTCallback(int messageSize) {
    String message;
    for (int i = 0; i < messageSize; i++) {
        message += (char)mqttClient.read();
    }
    messageData.processMessage(message); // The message components are stored within the connection where the messageReceived flag is set
}


void DashioMQTT::sendMessage(const String& message, MQTTTopicType topic) {
    if (mqttClient.connected()) {
        String publishTopic = dashioDevice->getMQTTTopic(username, topic);
        mqttClient.beginMessage(publishTopic, message.length(), false, MQTT_QOS, false); // reatined = false, duplicate = false
        mqttClient.print(message);
        mqttClient.endMessage();

        if (printMessages) {
            Serial.print(F("---- MQTT Sent ---- Topic: "));
            Serial.println(publishTopic);
            Serial.println(message);
        }
    }
}

void DashioMQTT::sendAlarmMessage(const String& message) {
    sendMessage(message, alarm_topic);
}

void DashioMQTT::sendWhoAnnounce() {
    sendMessage(dashioDevice->getWhoMessage(), announce_topic);
}

void DashioMQTT::run() {
    mqttClient.poll();
    if (mqttClient.connected()) {
        if (messageData.messageReceived) {
            messageData.messageReceived = false;

            if (printMessages) {
                Serial.println(messageData.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(messageData.control)));
            }

            switch (messageData.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            case config:
                dashioDevice->dashboardID = messageData.idStr;
                if (dashioDevice->configC64Str != NULL) {
                    sendMessage(dashioDevice->getC64ConfigMessage());
                } else {
                    if (processMQTTmessageCallback != NULL) {
                        processMQTTmessageCallback(&messageData);
                    }
                }
                break;
            default:
                if (processMQTTmessageCallback != NULL) {
                    processMQTTmessageCallback(&messageData);
                }
                break;
            }
        }
        
        messageData.checkBuffer();
    }
}

void DashioMQTT::hostConnect() { // Non-blocking
    // Setup MQTT Last Will and Testament message (Optional).
    String willTopic = dashioDevice->getMQTTTopic(username, will_topic);
    Serial.print(F("LWT topic: "));
    Serial.println(willTopic);

    String offlineMessage = dashioDevice->getOfflineMessage();
    mqttClient.beginWill(willTopic, offlineMessage.length(), true, MQTT_QOS);
    mqttClient.print(offlineMessage);
    mqttClient.endWill();
    Serial.print(F("LWT message: "));
    Serial.println(offlineMessage);
    
    mqttClient.setKeepAliveInterval(10000);
    mqttClient.setConnectionTimeout(10000);
    mqttClient.onMessage(messageReceivedMQTTCallback);

    Serial.print(F("Connecting to MQTT..."));
    mqttClient.setUsernamePassword(username, password);
    if (mqttClient.connect(mqttHost, mqttPort)) {
        Serial.println(F("connected"));

        // Subscribe to private MQTT connection
        String subscriberTopic = dashioDevice->getMQTTSubscribeTopic(username);
        mqttClient.subscribe(subscriberTopic, MQTT_QOS);
    
        // Send MQTT ONLINE and WHO messages to connection (Optional)
        // WHO is only required here if using the Dash server and it must be send to the ANNOUNCE topic
        sendMessage(dashioDevice->getOnlineMessage());
        sendMessage(dashioDevice->getWhoMessage(), announce_topic); // Update announce topic with new name

        if (dashStore != nullptr) {
            for (int i=0; i<dashStoreSize; i++) { // Announce control for data store on dash server
                sendMessage(dashioDevice->getDataStoreEnableMessage(dashStore[i]), announce_topic);
            }
        }
        
        if (reboot) {
            reboot = false;
            if (sendRebootAlarm) {
                sendAlarmMessage(dashioDevice->getAlarmMessage("ALX", "System Reboot", dashioDevice->name));
            }
        }
    } else {
        Serial.print(F("Failed - Try again in 10 seconds: "));
        Serial.println(mqttClient.connectError());
    }
}

void DashioMQTT::addDashStore(ControlType controlType, String controlID) {
    DashStore tempDashStore[dashStoreSize];
    
    if (dashStore != nullptr) {
        for (int i=0; i<dashStoreSize; i++) {
            tempDashStore[i] = dashStore[i];
        }
        delete[] dashStore;
    }

    dashStore = new DashStore[dashStoreSize + 1];
    for (int i=0; i<dashStoreSize; i++) {
        dashStore[i] = tempDashStore[i];
    }
    DashStore newDashStore = {controlType, controlID};
    dashStore[dashStoreSize] = newDashStore;
    dashStoreSize += 1;
}

void DashioMQTT::setCallback(void (*processIncomingMessage)(MessageData *connection)) {
    processMQTTmessageCallback = processIncomingMessage;
}

void DashioMQTT::setup(char *_username, char *_password) {
    username = _username;
    password = _password;
}

void DashioMQTT::checkConnection() {
    // Check and connect MQTT as necessary
    if (!mqttClient.connected()) {
        if (mqttConnectCount == 0) {
            hostConnect();
        }
        if (mqttConnectCount >= MQTT_RETRY_S) {
            mqttConnectCount = 0;
        } else {
            mqttConnectCount++;
        }
    }
}

void DashioMQTT::end() {
    sendMessage(dashioDevice->getOfflineMessage());
//    mqttClient.disconnect();
}

// ---------------------------------------- BLE ----------------------------------------
#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : bleService(SERVICE_UUID),
            bleReadCharacteristic(CHARACTERISTIC_UUID, BLEWriteWithoutResponse, BLE_MAX_SEND_MESSAGE_LENGTH),
            bleWriteCharacteristic(CHARACTERISTIC_UUID, BLENotify, BLE_MAX_SEND_MESSAGE_LENGTH) {
                                
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
    
    // Event driven reads.
    bleReadCharacteristic.setEventHandler(BLEWritten, onReadValueUpdate);

    // add the characteristic to the service
    bleService.addCharacteristic(bleReadCharacteristic);
    bleService.addCharacteristic(bleWriteCharacteristic);
}

MessageData DashioBLE::messageData(BLE_CONN, INCOMING_BUFFER_SIZE);

void DashioBLE::onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic) {
    // central wrote new value to characteristic
    int dataLength = characteristic.valueLength();
    char value[dataLength + 1];  // one byte more, to save the '\0' character!
    characteristic.readValue(value, dataLength);
    value[dataLength] = '\0';  // make sure to null-terminate!
    messageData.processMessage(value);
}

void DashioBLE::setCallback(void (*processIncomingMessage)(MessageData *connection)) {
    processBLEmessageCallback = processIncomingMessage;
}

void DashioBLE::begin() {
    if (BLE.begin()) {
        // set advertised local name and service UUID:
        String localName = F("DashIO_");
        localName += dashioDevice->type;
        Serial.print(F("BLE local name: "));
        Serial.println(localName);
        BLE.setLocalName(localName.c_str());
        BLE.setDeviceName(localName.c_str());
        
        BLE.setConnectable(true);
        BLE.setAdvertisedService(bleService);

        // add service
        BLE.addService(bleService);

        // start advertising
        BLE.advertise();
    } else {
        Serial.println(F("Starting BLE failed"));
    }
}

void DashioBLE::sendMessage(const String& message) {
    if (BLE.connected()) {
        int maxMessageLength = BLE_MAX_SEND_MESSAGE_LENGTH;
        
        if (message.length() <= maxMessageLength) {
            bleWriteCharacteristic.writeValue(message.c_str());
        } else {
            int messageLength = message.length();
            int numFullStrings = messageLength / maxMessageLength;
    
            String subStr((char *)0);
            subStr.reserve(maxMessageLength);
            
            int start = 0;
            for (unsigned int i = 0; i < numFullStrings; i++) {
                subStr = message.substring(start, start + maxMessageLength);
                bleWriteCharacteristic.writeValue(subStr.c_str());
                start += maxMessageLength;
            }
            if (start < messageLength) {
                subStr = message.substring(start);
                bleWriteCharacteristic.writeValue(subStr.c_str());
            }
        }
    
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(message);
        }
    }
}

void DashioBLE::run() {
    if (BLE.connected()) {
        BLE.poll(); // Required for event handlers
        if (messageData.messageReceived) {
            messageData.messageReceived = false;
    
            if (printMessages) {
                Serial.println(messageData.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(messageData.control)));
            }
    
            switch (messageData.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            case config:
                dashioDevice->dashboardID = messageData.idStr;
                if (dashioDevice->configC64Str != NULL) {
                    sendMessage(dashioDevice->getC64ConfigMessage());
                } else {
                    if (processBLEmessageCallback != NULL) {
                        processBLEmessageCallback(&messageData);
                    }
                }
                break;
            default:
                if (processBLEmessageCallback != NULL) {
                    processBLEmessageCallback(&messageData);
                }
                break;
            }
        }
        
        messageData.checkBuffer();
    }
}

bool DashioBLE::isConnected() {
    return BLE.connected();
}

void DashioBLE::end() {
    BLE.stopAdvertise();
    BLE.end();
}

String DashioBLE::macAddress() {
    return BLE.address();
}

#endif
// -------------------------------------------------------------------------------------

#endif
