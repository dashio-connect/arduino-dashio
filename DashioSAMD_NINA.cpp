#if defined ARDUINO_SAMD_NANO_33_IOT || defined ARDUINO_SAMD_MKRWIFI1010
//#ifdef ARDUINO_ARCH_SAMD

#include "DashioSAMD_NINA.h"

// WiFi
const int WIFI_CONNECT_TIMEOUT_MS = 5000; // 5s

// MQTT
const int  MQTT_QOS     = 1; // QoS = 1, as 2 causes subscribe to fail
const int  MQTT_RETRY_S = 10; // Retry after 10 seconds
const char *MQTT_SERVER = "dash.dashio.io";
const int  MQTT_PORT    = 8883;

// BLE
const int BLE_MAX_SEND_MESSAGE_LENGTH = 100;

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

    if (mqttConnection != NULL) {
        mqttConnection->begin();
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
    byte mac[6];
    WiFi.macAddress(mac);
    return mac;
}

String DashioWiFi::ipAddress() {
    static char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
    return ipStr;
}

// ---------------------------------------- TCP ----------------------------------------

DashioTCP::DashioTCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages) : wifiServer(_tcpPort), messageData(TCP_CONN) {
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
}

void DashioTCP::run() {
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

/*???
void DashioTCP::setupmDNSservice() {
    if (!client.begin("nano33iot")) {
       Serial.println(F("Error starting mDNS"));
       return;
    }
    Serial.println(F("mDNS started"));
    client.addService("DashIO", "tcp", tcpPort);
}

void DashioTCP::updatemDNS() {
    MDNS.update();
}
*/

// ---------------------------------------- MQTT ---------------------------------------

MessageData DashioMQTT::messageData(MQTT_CONN);

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, int _bufferSize, bool _sendRebootAlarm, bool _printMessages) {
    dashioDevice = _dashioDevice;
    bufferSize = _bufferSize;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
    mqttClient = PubSubClient(wifiClient);
}


void DashioMQTT::messageReceivedMQTTCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    messageData.processMessage(message); // The message components are stored within the connection where the messageReceived flag is set
}

void DashioMQTT::sendMessage(const String& message, MQTTTopicType topic) {
    if (mqttClient.connected()) {
        String publishTopic = dashioDevice->getMQTTTopic(username, topic);
        mqttClient.publish(publishTopic.c_str(), message.c_str());

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

void DashioMQTT::run() {
    if (mqttClient.loop()) {
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
            default:
                if (processMQTTmessageCallback != NULL) {
                    processMQTTmessageCallback(&messageData);
                }
                break;
            }
        }
    }
}

void DashioMQTT::hostConnect() { // Non-blocking
    String willTopic = dashioDevice->getMQTTTopic(username, will_topic);

    Serial.print(F("Connecting to MQTT..."));
    String offlineMessage = dashioDevice->getOfflineMessage();
    if (mqttClient.connect(dashioDevice->deviceID.c_str(), username, password, willTopic.c_str(), MQTT_QOS, false, offlineMessage.c_str())) {
        Serial.println(F("connected"));

        // Subscribe to private MQTT connection
        String subscriberTopic = dashioDevice->getMQTTSubscribeTopic(username);
        mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS);

        // Send MQTT ONLINE and WHO messages to connection (Optional)
        // WHO is only required here if using the Dash server and it must be send to the ANNOUNCE topic
        sendMessage(dashioDevice->getOnlineMessage());
        sendMessage(dashioDevice->getWhoMessage(), announce_topic); // Update announce topic with new name

        if (reboot) {
            reboot = false;
            if (sendRebootAlarm) {
                sendAlarmMessage(dashioDevice->getAlarmMessage("ALX", "System Reboot", dashioDevice->name));
            }
        }
    } else {
        Serial.print(F("Failed - Try again in 10 seconds: "));
        Serial.println(mqttClient.state());
    }
}

void DashioMQTT::setCallback(void (*processIncomingMessage)(MessageData *connection)) {
    processMQTTmessageCallback = processIncomingMessage;
}

void DashioMQTT::setup(char *_username, char *_password) {
    username = _username;
    password = _password;
}

void DashioMQTT::begin() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(messageReceivedMQTTCallback);
    mqttClient.setBufferSize(bufferSize);
    mqttClient.setSocketTimeout(10);
    mqttClient.setKeepAlive(10);
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
    mqttClient.disconnect();
}

// ---------------------------------------- BLE ----------------------------------------

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : bleService(SERVICE_UUID),
                                                                                   bleCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWriteWithoutResponse | BLENotify, BLE_MAX_SEND_MESSAGE_LENGTH, false) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;

    // Event driven reads.
    bleCharacteristic.setEventHandler(BLEWritten, onReadValueUpdate);

    // add the characteristic to the service
    bleService.addCharacteristic(bleCharacteristic);
}

MessageData DashioBLE::messageData(BLE_CONN);

void DashioBLE::onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic) {
    // central wrote new value to characteristic
    int dataLength = characteristic.valueLength();
    char data[dataLength];
    int finalLength = characteristic.readValue(data, dataLength);
    for (int i = 0; i < dataLength; i++) {
        if (messageData.processChar(data[i])) {
            messageData.messageReceived = true;
        }
    }
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
            bleCharacteristic.writeValue(message.c_str());
        } else {
            int messageLength = message.length();
            int numFullStrings = messageLength / maxMessageLength;
    
            String subStr((char *)0);
            subStr.reserve(maxMessageLength);
            
            int start = 0;
            for (unsigned int i = 0; i < numFullStrings; i++) {
                subStr = message.substring(start, start + maxMessageLength);
                bleCharacteristic.writeValue(subStr.c_str());
                start += maxMessageLength;
            }
            if (start < messageLength) {
                subStr = message.substring(start);
                bleCharacteristic.writeValue(subStr.c_str());
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
            default:
                if (processBLEmessageCallback != NULL) {
                    processBLEmessageCallback(&messageData);
                }
                break;
            }
        }
    }
}

bool DashioBLE::connected() {
    return BLE.connected();
}

void DashioBLE::end() {
    BLE.stopAdvertise();
    BLE.end();
}

String DashioBLE::macAddress() {
    return BLE.address();
}

// -------------------------------------------------------------------------------------

#endif
