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

#if defined ESP32 || defined ESP8266

#include "DashioESP.h"

#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes
#ifdef ESP32
    #define C64_MAX_LENGHT 1000
#elif ESP8266
    #define C64_MAX_LENGHT 500
#endif

const int INCOMING_BUFFER_SIZE = 512;

// MQTT
const int MQTT_QOS = 2;
const int MQTT_RETRY_S = 10; // Retry after 10 seconds
const int MQTT_CLIENT_BUFFER_SIZE = 2048;
const int MQTT_SEND_WAIT_MS = 1000;
const int MQTT_SEND_BUFFER_MIN = 1024;

// BLE
const int BLE_MAX_SEND_MESSAGE_LENGTH = 185; // 185 for iPhone 6, but can be up to 517

// ---------------------------------------- WiFi ---------------------------------------

bool DashioWiFi::oneSecond = false;

DashioWiFi::DashioWiFi(DashioDevice *_dashioDevice) {
    dashioDevice = _dashioDevice;
    
#ifdef ESP32
    xTaskCreatePinnedToCore(this->wifiOneSecondTask, "OneSecTask", 2048, this, 0, &wifiOneSecTaskHandle, 0);
#elif ESP8266
    timer.every(1000, onTimerCallback); // 1000ms
#endif
}

#ifdef ESP32
void DashioWiFi::wifiOneSecondTask(void *parameter) {
    for(;;) {
        oneSecond = true;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#elif ESP8266
bool DashioWiFi::onTimerCallback(void *argument) { // Timer Interrupt
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}
#endif

void DashioWiFi::attachConnection(DashioTCP *_tcpConnection) {
    tcpConnection = _tcpConnection;
}

void DashioWiFi::detachTcp() {
    tcpConnection = nullptr;
}

void DashioWiFi::attachConnection(DashioMQTT *_mqttConnection) {
    mqttConnection = _mqttConnection;
}

void DashioWiFi::detachMqtt() {
    mqttConnection = nullptr;
}

void DashioWiFi::setOnConnectCallback(void (*connectCallback)(void)) {
    wifiConnectCallback = connectCallback;
}

void DashioWiFi::begin(char *ssid, char *password) {
    // Below is required to get WiFi to connect reliably on ESP32
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    WiFi.begin(ssid, password);
    wifiConnectCount = 1;
}

void DashioWiFi::run() {
#ifdef ESP8266
    timer.tick();
#endif
    
    if (mqttConnection != nullptr) {
        mqttConnection->run();
    }
    
    if (tcpConnection != nullptr) {
        tcpConnection->run();
    }

    if (oneSecond) {
        oneSecond = false;

        // First, check WiFi and connect if necessary
        if (WiFi.status() != WL_CONNECTED) {
            wifiConnectCount++;
            Serial.print(F("Connecting to Wi-Fi "));
            Serial.println(String(wifiConnectCount));

            if (mqttConnection != nullptr) {
                mqttConnection->state = notReady;
            }

            if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP32. Sometimes ESP32's WiFi gets tied up in a knot.
                if (dashioDevice != nullptr) {
                    dashioDevice->onStatusCallback(wifiDisconnected);
                } else if (mqttConnection != nullptr) { // TODO??? can remove in future
                    mqttConnection->dashioDevice->onStatusCallback(wifiDisconnected);
                } else if (tcpConnection != nullptr) { // TODO??? can remove in future
                    tcpConnection->dashioDevice->onStatusCallback(wifiDisconnected);
                }

                ESP.restart();
            }
        } else {
            // WiFi OK
            if (wifiConnectCount > 0) {
                wifiConnectCount = 0; // So that we only execute the following once after WiFI connection
                Serial.print("Connected with IP: ");
                Serial.println(WiFi.localIP());

                if (wifiConnectCallback != nullptr) { // Deprtecated
                    wifiConnectCallback();
                }

                if (dashioDevice != nullptr) {
                    dashioDevice->onStatusCallback(wifiConnected);
                } else if (mqttConnection != nullptr) { // TODO??? can remove in future
                    mqttConnection->dashioDevice->onStatusCallback(wifiConnected);
                } else if (tcpConnection != nullptr) { // TODO??? can remove in future
                    tcpConnection->dashioDevice->onStatusCallback(wifiConnected);
                }

                if (tcpConnection != nullptr) {
                    tcpConnection->begin();
                    tcpConnection->setupmDNSservice(macAddress());
                }
                
                if (mqttConnection != nullptr) {
                    mqttConnection->begin();
                }
            }
            if (mqttConnection != nullptr) {
#ifdef ESP32
                if (mqttConnection->esp32_mqtt_blocking) {
                    mqttConnection->checkConnection();
                }
#elif ESP8266
                mqttConnection->checkConnection();
#endif

            }
        }
    }
}

void DashioWiFi::end() {
    if (tcpConnection != nullptr) {
        tcpConnection->end();
    }
    
    if (mqttConnection != nullptr) {
        mqttConnection->end();
    }

    WiFi.disconnect();
}

String DashioWiFi::macAddress() {
#ifdef ESP32
  #if ESP_IDF_VERSION_MAJOR >= 5
    return Network.macAddress();
  #else
    return WiFi.macAddress();
  #endif
#elif ESP8266
    return WiFi.macAddress();
#endif
}

String DashioWiFi::ipAddress() {
    IPAddress ip = WiFi.localIP();
    static char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return ipStr;
}

// --------------------------------------- Soft AP -------------------------------------

bool DashioSoftAP::begin(const String& password) {
    IPAddress IP = {192, 168, 68, 100};
    IPAddress NMask = {255, 255, 255, 0};

    Serial.println(F("Starting soft-AP"));

#ifdef ESP32
    bool result = WiFi.softAP("Dash_Provision", password.c_str(), 1, 0, 1);
#elif ESP8266
    bool result = WiFi.softAP(F("Dash_Provision"), password, 1, 0, 1);
#endif
    if (result == true) {
        WiFi.softAPConfig(IP, IP, NMask);

        Serial.print(F("AP IP address: "));
        Serial.println(WiFi.softAPIP());
        if (tcpConnection != nullptr) {
            originalTCPport = tcpConnection->tcpPort;
            tcpConnection->setPort(SOFT_AP_PORT);
            tcpConnection->begin();
        }
    }
    return result;
}

void DashioSoftAP::attachConnection(DashioTCP *_tcpConnection) {
    tcpConnection = _tcpConnection;
}

void DashioSoftAP::end() {
    WiFi.softAPdisconnect(false); // false = turn off soft AP
    tcpConnection->setPort(originalTCPport);
}

bool DashioSoftAP::isConnected() {
    return (WiFi.softAPgetStationNum() > 0);
}

void DashioSoftAP::run() {
    if (tcpConnection != nullptr) {
        tcpConnection->run();
    }
}

// ---------------------------------------- TCP ----------------------------------------

#ifdef ESP32
DashioTCP::DashioTCP(DashioDevice *_dashioDevice, bool _printMessages, uint16_t _tcpPort, uint8_t _maxTCPclients) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
    wifiServer = WiFiServer(_tcpPort);

    maxTCPclients = _maxTCPclients;
    tcpClients = new TCPclient[_maxTCPclients];
}
#elif ESP8266
DashioTCP::DashioTCP(DashioDevice *_dashioDevice, bool _printMessages, uint16_t _tcpPort, uint8_t _maxTCPclients) : wifiServer(_tcpPort) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;

    maxTCPclients = _maxTCPclients;
    tcpClients = new TCPclient[_maxTCPclients];
}
#endif

void DashioTCP::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processTCPmessageCallback = processIncomingMessage;
}

void DashioTCP::setPort(uint16_t _tcpPort) {
    tcpPort = _tcpPort;
}

void DashioTCP::begin() {
    wifiServer.begin(tcpPort);
}

uint8_t DashioTCP::hasClient() {
    uint8_t rVal = 0;
    for (int i = 0; i < maxTCPclients; i++) {
        TCPclient *tcpClientPtr = &tcpClients[i];
        if (tcpClientPtr->client) {
            rVal = tcpClientPtr->client.connected();
        }
    }
    return rVal;
}

void DashioTCP::sendMessage(const String& message, uint8_t index) {
    if (index < maxTCPclients) {
        WiFiClient *clientPtr = &tcpClients[index].client;
        if (clientPtr->connected()) {
            clientPtr->print(message);
            if (printMessages) {
                Serial.println(F("---- TCP Sent ----"));
                Serial.println(message);
            }
        }
    }
}

void DashioTCP::sendMessage(const String& message) {
    for (int i = 0; i < maxTCPclients; i++) {
        sendMessage(message, i);
    }
}

void DashioTCP::setupmDNSservice(const String& id) {
    char charBuf[id.length()];
    id.toCharArray(charBuf, id.length() + 1);
    if (!MDNS.begin(charBuf)) {
        if (printMessages) {
            Serial.println(F("Error starting mDNS"));
        }
        return;
    }
    if (printMessages) {
        Serial.print(F("mDNS started: "));
        Serial.println(String(tcpPort));
    }
    MDNS.addService("DashIO", "tcp", tcpPort);
}
    
void DashioTCP::end() {
    for (int i = 0; i < maxTCPclients; i++) {
        if (tcpClients[i].client) {
            tcpClients[i].client.stop();
        }
    }

#ifdef ESP8266
    MDNS.close();
#endif
    MDNS.end();
}

void DashioTCP::processConfig(uint16_t index) {
    sendMessage(dashioDevice->getC64ConfigBaseMessage(), index);
    
    int c64Length = strlen_P(dashioDevice->configC64Str);
    int length = 0;
    String message = "";
    for (int k = 0; k < c64Length; k++) {
        char myChar = pgm_read_byte_near(dashioDevice->configC64Str + k);

        message += myChar;
        length++;
        if (length == C64_MAX_LENGHT) {
            sendMessage(message, index);
            message = "";
            length = 0;
        }
    }
    message += String(END_DELIM);
    sendMessage(message, index);
}

bool DashioTCP::checkTCP(int index) {
    TCPclient *tcpClientPtr = &tcpClients[index];
    if (tcpClientPtr->client.connected()) {
        while (tcpClientPtr->client.available()>0) {
            char c = tcpClientPtr->client.read();
            if (tcpClientPtr->data.processChar(c)) {
                tcpClientPtr->data.connectionHandle = index; // So we have a reference to the index in the MessageData

                if (printMessages) {
                    Serial.println(tcpClientPtr->data.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(tcpClientPtr->data.control)));
                }
                
                switch (tcpClientPtr->data.control) {
                case who:
                    sendMessage(dashioDevice->getWhoMessage(), index);
                    break;
                case connect:
                    sendMessage(dashioDevice->getConnectMessage(), index);
                    break;
                case config:
                    dashioDevice->dashboardID = tcpClientPtr->data.idStr;
                    if (dashioDevice->configC64Str != nullptr) {
                        processConfig(index);
                    } else {
                        if (processTCPmessageCallback != nullptr) {
                            processTCPmessageCallback(&tcpClientPtr->data);
                        }
                    }
                    break;
                default:
                    if (processTCPmessageCallback != nullptr) {
                        processTCPmessageCallback(&tcpClientPtr->data);
                    }
                    break;
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

void DashioTCP::run() {
    WiFiClient newClient = wifiServer.accept();
    if (newClient) {
        for (int i = 0; i < maxTCPclients; ++i) {
            if (!tcpClients[i].client) {
                newClient.setTimeout(2000);
                tcpClients[i].client = newClient;
                break;
            }
        }
    }

    for (int i = 0; i < maxTCPclients; i++) {
        if (!checkTCP(i)) {
            if (tcpClients[i].client) {
                tcpClients[i].client.stop(); // This doesn't every get called
//                tcpClients[i].client = 0;
            }
        }
    }
    
#ifdef ESP8266
    MDNS.update();
#endif
}

// ---------------------------------------- MQTT ---------------------------------------

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages) : mqttClient(MQTT_CLIENT_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;

#ifdef ESP32
    xTaskCreatePinnedToCore(this->checkConnectionTask, "CheckConnTask", 10000, this, 0, &mqttConnectTaskHandle, 0);
#endif
}

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages, int _mqttBufferSize) : mqttClient(MQTT_CLIENT_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
    if (_mqttBufferSize >= MQTT_SEND_BUFFER_MIN) {
        mqttBuffersize = _mqttBufferSize;
        mqttSendBuffer.reserve(_mqttBufferSize);
    }

#ifdef ESP32
    xTaskCreatePinnedToCore(this->checkConnectionTask, "TasCheckConnTaskk1", 10000, this, 0, &mqttConnectTaskHandle, 0);
#endif
}

MessageData DashioMQTT::data(MQTT_CONN, INCOMING_BUFFER_SIZE);

void DashioMQTT::messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length) {
    data.processMessage(String(payload)); // The message components are stored within the connection where the messageReceived flag is set
}

void DashioMQTT::checkAndSendMQTTbuffer() {
    if (mqttSendBuffer.length() > 0) {
        unsigned long timeNow = millis();
        long timeSinceLastMessage = timeNow - lastSentMessageTime;
        if (timeSinceLastMessage < 0) {timeSinceLastMessage = MQTT_SEND_WAIT_MS;}
        if (timeSinceLastMessage >= MQTT_SEND_WAIT_MS) {
            lastSentMessageTime = timeNow;
            publishMessage(mqttSendBuffer, data_topic);
            mqttSendBuffer.clear();
        }
    }
}

void DashioMQTT::publishMessage(const String& message, MQTTTopicType topic) {
    if (mqttClient.connected()) {
        String publishTopic = dashioDevice->getMQTTTopic(username, topic);
        mqttClient.publish(publishTopic.c_str(), message.c_str(), false, MQTT_QOS);

        if (printMessages) {
            Serial.print(F("---- MQTT Sent ---- Topic: "));
            Serial.println(publishTopic);
            Serial.println(message);
        }
    }
}

void DashioMQTT::sendMessage(const String& message, MQTTTopicType topic) {
    if (mqttBuffersize >= MQTT_SEND_BUFFER_MIN && topic == data_topic) {
        if (message.length() < (mqttBuffersize - mqttSendBuffer.length())) {
            mqttSendBuffer += message;
            checkAndSendMQTTbuffer();
        }
    } else {
        publishMessage(message, topic);
    }
}

void DashioMQTT::sendAlarmMessage(const String& message) {
    sendMessage(message, alarm_topic);
}

void DashioMQTT::sendWhoAnnounce() {
    sendMessage(dashioDevice->getWhoMessage(), announce_topic);
}

void DashioMQTT::processConfig() {
    sendMessage(dashioDevice->getC64ConfigBaseMessage());
    
    int c64Length = strlen_P(dashioDevice->configC64Str);
    int length = 0;
    String message = "";
    for (int k = 0; k < c64Length; k++) {
        char myChar = pgm_read_byte_near(dashioDevice->configC64Str + k);

        message += myChar;
        length++;
        if (length == (MQTT_CLIENT_BUFFER_SIZE / 2)) {
            sendMessage(message);
            message = "";
            length = 0;
        }
    }
    message += String(END_DELIM);
    sendMessage(message);
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

void DashioMQTT::setupLWT() {
    // Setup MQTT Last Will and Testament message (Optional). Default keep alive time is 10s

    String willTopic = dashioDevice->getMQTTTopic(username, will_topic);
    String offlineMessage = dashioDevice->getOfflineMessage();
    mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);

    if (printMessages) {
        Serial.print(F("LWT topic: "));
        Serial.println(willTopic);
        
        Serial.print(F("LWT message: "));
        Serial.println(offlineMessage);
    }
}

void DashioMQTT::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processMQTTmessageCallback = processIncomingMessage;
}
    
void DashioMQTT::setup(char *_username, char *_password) {
    username = _username;
    password = _password;
    state = notReady;
}

void DashioMQTT::begin() {
    if (wifiSetInsecure) {
        wifiClient.setInsecure();
    }

    mqttClient.begin(mqttHost, mqttPort, wifiClient);
    mqttClient.setOptions(10, true, 10000);  // 10s timeout
    mqttClient.onMessageAdvanced(messageReceivedMQTTCallback);
  
    setupLWT(); // Once the deviceID is known
    state = disconnected;
}

void DashioMQTT::onConnected() {
    // Subscribe to private MQTT connection
    String subscriberTopic = dashioDevice->getMQTTSubscribeTopic(username);
    mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS); // ... and subscribe

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
    
    dashioDevice->onStatusCallback(mqttConnected);
}

void DashioMQTT::hostConnect() {
    Serial.print(F("Connecting MQTT..."));
    state = connecting;

/* In case deviceID.c_str() gives trouble
    int idLen = dashioDevice->deviceID.length() + 1;
    char clientID[idLen];
    dashioDevice->deviceID.toCharArray(clientID, idLen);
*/
    if (mqttClient.connect(dashioDevice->deviceID.c_str(), username, password, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
        if (printMessages) {
            Serial.println(F("connected"));
        }
        state = serverConnected;
    } else {
        if (printMessages) {
            Serial.print(F("failed - Try again in 10 seconds. E = "));
            Serial.println(String(mqttClient.lastError()) + "  R = " + mqttClient.returnCode());
            // Invalid URL or port => E = -3  R = 0
            // Invalid username or password => E = -10  R = 5
            // Invalid SSL record => E = -5  R = 6
        }
        state = disconnected;
    }
}

void DashioMQTT::checkConnection() {
    // Check and connect MQTT as necessary
    if (WiFi.status() == WL_CONNECTED) {
        if (state == disconnected) {
            if (mqttConnectCount == 0) {
                hostConnect();
            }
            if (mqttConnectCount >= MQTT_RETRY_S) {
                mqttConnectCount = 0;

                dashioDevice->onStatusCallback(mqttDisconnected);
            } else {
                mqttConnectCount++;
            }
        } else if (state == serverConnected) {
            mqttConnectCount = 0;
        }
    }
}
    
#ifdef ESP32
void DashioMQTT::checkConnectionTask(void * parameter) {
    DashioMQTT *mqttConn = (DashioMQTT *) parameter;
    for(;;) {
        if (mqttConn != nullptr) {
            if (!mqttConn->esp32_mqtt_blocking) {
                mqttConn->checkConnection();
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif

void DashioMQTT::run() {
    if (mqttClient.connected()) {
        mqttClient.loop();

        if (state == serverConnected) {
            onConnected();
            state = subscribed;
        }

        if (data.messageReceived) {
            data.messageReceived = false;

            if (printMessages) {
                Serial.println(data.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(data.control)));
            }

            switch (data.control) {
                case who:
                    sendMessage(dashioDevice->getWhoMessage());
                    break;
                case connect:
                    sendMessage(dashioDevice->getConnectMessage());
                    break;
                case config:
                    dashioDevice->dashboardID = data.idStr;
                    if (dashioDevice->configC64Str != nullptr) {
                        processConfig();
                    } else {
                        if (processMQTTmessageCallback != nullptr) {
                            processMQTTmessageCallback(&data);
                        }
                    }
                    break;
                default:
                    if (processMQTTmessageCallback != nullptr) {
                        processMQTTmessageCallback(&data);
                    }
                    break;
            }
        }

        data.checkBuffer();
        checkAndSendMQTTbuffer();
    } else {
        if ((state == serverConnected) or (state == subscribed)) {
            state = disconnected;
        }
    }
}

void DashioMQTT::end() {
    sendMessage(dashioDevice->getOfflineMessage());
    mqttClient.disconnect();
}

// ---------------------------------------- BLE ----------------------------------------
#ifdef ESP32
BLEclientHolder *DashioBLE::bleClients = nullptr;
uint8_t DashioBLE::maxBLEclients = 1;
bool DashioBLE::printMessages = false;
uint32_t DashioBLE::passKey = 0;

class ServerCallbacks: public NimBLEServerCallbacks {
public:
    ServerCallbacks(DashioBLE * local_DashioBLE): local_DashioBLE(local_DashioBLE) { }
    DashioBLE *local_DashioBLE = nullptr;

    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc *desc) {
        ESP_LOGI(DTAG, "BLE Client Connected, handle: %d", desc->conn_handle);
        if (!local_DashioBLE->setConnectionActive(desc->conn_handle)) {
            ESP_LOGI(DTAG, "No connections left for handle: %d", desc->conn_handle);
        }

        if (pServer->getConnectedCount() < local_DashioBLE->maxBLEclients) {
            NimBLEDevice::startAdvertising(); // Keep advertising for more connections
        }
    }

    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc *desc) {
        ESP_LOGI(DTAG, "BLE Client Disconnected, handle: %d", desc->conn_handle);
        local_DashioBLE->setConnectionInactive(desc->conn_handle);
    }

    // Security callback functions
    uint32_t onPassKeyRequest() {
        ESP_LOGI(DTAG,"Server Passkey Request: %d", local_DashioBLE->passKey);
        return local_DashioBLE->passKey;
    };

    void onAuthenticationComplete(ble_gap_conn_desc *desc) {
        if (desc->sec_state.authenticated) {
            ESP_LOGI(DTAG, "BLE Authenticated");
        } else {
            ESP_LOGI(DTAG, "BLE Authentication FAIL");
            local_DashioBLE->setConnectionAuthState(desc->conn_handle, BLE_AUTH_FAIL);
        }
        if (desc->sec_state.encrypted) {
            ESP_LOGI(DTAG, "BLE Encrypted");
        }
        if (desc->sec_state.bonded) {
            ESP_LOGI(DTAG, "BLE Bonded");
            local_DashioBLE->setConnectionAuthState(desc->conn_handle, BLE_AUTH_REQ_CONN);
        }
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(DashioBLE * local_DashioBLE): local_DashioBLE(local_DashioBLE) { }
    DashioBLE *local_DashioBLE = nullptr;

    void onWrite(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc) { // BLE callback for when a message is received
        std::string payload = pCharacteristic->getValue();
        local_DashioBLE->data.processMessage(payload.c_str(), desc->conn_handle); /// The message components are stored within the connection where the messageReceived flag is set
    }
};

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : data(BLE_CONN, INCOMING_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
    maxBLEclients = 1;
    
    initialiseClientHolders();
}

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages, uint8_t _maxBLEclients) : data(BLE_CONN, INCOMING_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
    maxBLEclients = _maxBLEclients;
    
    initialiseClientHolders();
}

void DashioBLE::bleNotifyValue(const String& message) {
    pCharacteristic->setValue(message);
    pCharacteristic->notify();
}

void DashioBLE::sendMessage(const String& message) {
    if (isConnected()) {
        int maxMessageLength = NimBLEDevice::getMTU() - 3;
        
        if (message.length() <= maxMessageLength) {
            bleNotifyValue(message);
        } else {
            int messageLength = message.length();
            int numFullStrings = messageLength / maxMessageLength;
    
            String subStr((char *)0);
            subStr.reserve(maxMessageLength);
            
            int start = 0;
            for (unsigned int i = 0; i < numFullStrings; i++) {
                subStr = message.substring(start, start + maxMessageLength);
                bleNotifyValue(subStr);
                start += maxMessageLength;
            }
            if (start < messageLength) {
                subStr = message.substring(start);
                bleNotifyValue(subStr);
            }
        }
    
        if (printMessages) {
            Serial.println(F("---- BLE Sent ----"));
            Serial.println(message);
        }
    }
}

void DashioBLE::processConfig() {
    sendMessage(dashioDevice->getC64ConfigBaseMessage());
    
    int c64Length = strlen_P(dashioDevice->configC64Str);
    int length = 0;
    String message = "";
    for (int k = 0; k < c64Length; k++) {
        char myChar = pgm_read_byte_near(dashioDevice->configC64Str + k);

        message += myChar;
        length++;
        if (length == C64_MAX_LENGHT) {
            sendMessage(message);
            message = "";
            length = 0;
        }
    }
    message += String(END_DELIM);
    sendMessage(message);
}

void DashioBLE::run() {
    if (secureBLE && (bleClients != nullptr)) {
        for (int i = 0; i < maxBLEclients; i++) {
            if (bleClients[i].authState == BLE_AUTH_REQ_CONN) {
                bleClients[i].authState = BLE_AUTHENTICATED;
                sendMessage(dashioDevice->getConnectMessage()); /// This wil go to all BLE clients as Arduino NimBLE doesn't yet allow messagein individual clients
                break; // Remove break when NimBLE can send to specific client/connectionHandle
            }
        }
    }
    
    if (data.messageReceived) {
        data.messageReceived = false;
        
        if (printMessages) {
            Serial.println(data.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(data.control)));
        }
        
        bool startAuth = false;
        
        switch (data.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                if (secureBLE && (bleClients != nullptr)) {
                    for (int i = 0; i < maxBLEclients; i++) {
                        if (bleClients[i].connectionHandle == data.connectionHandle) {
                            if (bleClients[i].authState == BLE_NOT_AUTH) {
                                startAuth = true;
                            }
                        }
                    }
                }
                if (startAuth) {
                    NimBLEDevice::startSecurity(data.connectionHandle);
                } else {
                    sendMessage(dashioDevice->getConnectMessage());
                }
                break;
            case config:
                dashioDevice->dashboardID = data.idStr;
                if (dashioDevice->configC64Str != nullptr) {
                    processConfig();
                } else {
                    if (processBLEmessageCallback != nullptr) {
                        processBLEmessageCallback(&data);
                    }
                }
                break;
            default:
                if (processBLEmessageCallback != nullptr) {
                    processBLEmessageCallback(&data);
                }
                break;
        }
    }
    
    data.checkBuffer();
}

void DashioBLE::end() {
    NimBLEDevice::deinit(true);
}

void DashioBLE::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processBLEmessageCallback = processIncomingMessage;
}
    
void DashBLE::setPassKey(uint32_t _passKey) {
    passKey = _passKey;
    secureBLE = (String(passKey).length() == 6);
    
    // Setup BLE security (optional)
    if (secureBLE) {
        NimBLEDevice::setSecurityAuth(true, true, true); // bool bonding, bool mitm (man in the middle), bool sc
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // uint8_t iocap Sets the Input/Output capabilities of this device.
    }
}
        
void DashioBLE::begin(uint32_t _passKey) {
    String localName = F("DashIO_");
    localName += dashioDevice->type;
    NimBLEDevice::init(localName.c_str());
    NimBLEDevice::setMTU(BLE_MAX_SEND_MESSAGE_LENGTH);
    
    if ((String(_passKey).length() == 6)) {
        setPassKey(_passKey);
    } else if ((String(passKey).length() == 6)) {
        setPassKey(passKey); // in case passKey was set BEFORE NimBLEDevice::init
    }

    // Setup server, service and characteristic
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
    pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
    pService->start();
    
    // Setup BLE advertising
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->start();
}
    
String DashioBLE::macAddress() {
    BLEAddress bdAddr = NimBLEDevice::getAddress();
    return bdAddr.toString().c_str();
}

void DashioBLE::advertise() {
    pAdvertising->start();
}

bool DashioBLE::isConnected() {
    if (pServer != nullptr) {
        return (pServer->getConnectedCount() > 0);
    } else {
        return false;
    }
}

void DashioBLE::initialiseClientHolders() {
    if (bleClients == nullptr) {
        bleClients = new BLEclientHolder(maxBLEclients);
    }
}

bool DashioBLE::setConnectionActive(uint16_t conn_handle) {
    bool success = false;
    if (bleClients != nullptr) {
        for (int i = 0; i < maxBLEclients; i++) { // Just incase it already exists
            if (bleClients[i].connectionHandle == conn_handle) {
                bleClients[i].active = true;
                success = true;
                break;
            }
        }

        if (!success) { // Find an inactive con holder and use it
            for (int i = 0; i < maxBLEclients; i++) {
                if (bleClients[i].active == false) {
                    bleClients[i].connectionHandle = conn_handle;
                    bleClients[i].active = true;
                    success = true;
                    break;
                }
            }
        }
    }
    return success;
}

void DashioBLE::setConnectionInactive(uint16_t conn_handle) {
    if (bleClients != nullptr) {
        for (int i = 0; i < maxBLEclients; i++) {
            if (bleClients[i].connectionHandle == conn_handle) {
                bleClients[i].connectionHandle = -1;
                bleClients[i].active = false;
                bleClients[i].authState = BLE_NOT_AUTH;
            }
        }
    }
}

void DashioBLE::setConnectionAuthState(uint16_t conn_handle, BLEauthState authState) {
    if (bleClients != nullptr) {
        for (int i = 0; i < maxBLEclients; i++) {
            if (bleClients[i].connectionHandle == conn_handle) {
                bleClients[i].authState = authState;
            }
        }
    }
}

// -------------------------------------------------------------------------------------

#endif
#endif
