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

#define MQTT_BUFFER_SIZE 2048
#define INCOMING_BUFFER_SIZE 512

// MQTT
const int MQTT_QOS     = 2;
const int MQTT_RETRY_S = 10; // Retry after 10 seconds

// BLE
const int BLE_MAX_SEND_MESSAGE_LENGTH = 185; // 185 for iPhone 6, but can be up to 517

// ---------------------------------------- WiFi ---------------------------------------

bool DashioWiFi::oneSecond = false;

// Timer Interrupt
bool DashioWiFi::onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

void DashioWiFi::attachConnection(DashioTCP *_tcpConnection) {
    tcpConnection = _tcpConnection;
}

void DashioWiFi::detachTcp() {
    tcpConnection = NULL;
}

void DashioWiFi::attachConnection(DashioMQTT *_mqttConnection) {
    mqttConnection = _mqttConnection;
}

void DashioWiFi::detachMqtt() {
    mqttConnection = NULL;
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

    timer.every(1000, onTimerCallback); // 1000ms
}

void DashioWiFi::run() {
    timer.tick();
    
    if (mqttConnection != NULL) {
        mqttConnection->run();
    }
    
    if (tcpConnection != NULL) {
        tcpConnection->run();
    }

    if (oneSecond) {
        oneSecond = false;

        // First, check WiFi and connect if necessary
        if (WiFi.status() != WL_CONNECTED) {
            wifiConnectCount++;
            Serial.print(F("Connecting to Wi-Fi "));
            Serial.println(String(wifiConnectCount));

            if (mqttConnection != NULL) {
                mqttConnection->state = notReady;
            }

            if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP32. Sometimes ESP32's WiFi gets tied up in a knot.
                ESP.restart();
            }
        } else {
            // WiFi OK
            if (wifiConnectCount > 0) {
                wifiConnectCount = 0; // So that we only execute the following once after WiFI connection
                Serial.print("Connected with IP: ");
                Serial.println(WiFi.localIP());

                if (wifiConnectCallback != NULL) {
                    wifiConnectCallback();
                }

                if (tcpConnection != NULL) {
                    tcpConnection->begin();
                    tcpConnection->setupmDNSservice(WiFi.macAddress());
                }
                
                if (mqttConnection != NULL) {
                    mqttConnection->begin();
                }
            }
            if (mqttConnection != NULL) {
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
    if (tcpConnection != NULL) {
        tcpConnection->end();
    }
    
    if (mqttConnection != NULL) {
        mqttConnection->end();
    }

    WiFi.disconnect();
}

String DashioWiFi::macAddress() {
    return WiFi.macAddress();
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
        if (tcpConnection != NULL) {
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
    if (tcpConnection != NULL) {
        tcpConnection->run();
    }
}

// ---------------------------------------- TCP ----------------------------------------

#ifdef ESP32
DashioTCP::DashioTCP(DashioDevice *_dashioDevice, bool _printMessages, uint16_t _tcpPort) : data(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
    wifiServer = WiFiServer(_tcpPort);
}
#elif ESP8266
DashioTCP::DashioTCP(DashioDevice *_dashioDevice, bool _printMessages, uint16_t _tcpPort) : wifiServer(_tcpPort), data(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
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
    uint8_t rVal;
    if (!client) {
        rVal = 0;
    }
    else {
        rVal = client.connected();
    }
    return rVal;
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
#ifdef ESP8266
    MDNS.close();
#endif
    MDNS.end();
}

void DashioTCP::processConfig() {
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
    if (message.length() > 0) {
        sendMessage(message);
    }
    sendMessage(String('\n'));
}

void DashioTCP::run() {
    if (!client) {
        client = wifiServer.accept();
        client.setTimeout(2000);
    } else {
       if (client.connected()) {
            while (client.available()>0) {
                char c = client.read();
                if (data.processChar(c)) {
                    if (printMessages) {
                        Serial.println(data.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(data.control)));
                    }
                    
                    if (passThrough) {
                        if (processTCPmessageCallback != nullptr) {
                            processTCPmessageCallback(&data);
                        }
                    } else {
                        switch (data.control) {
                        case who:
                            sendMessage(dashioDevice->getWhoMessage());
                            break;
                        case connect:
                            sendMessage(dashioDevice->getConnectMessage());
                            break;
                        case config:
                            dashioDevice->dashboardID = data.idStr;
                            if (dashioDevice->configC64Str != NULL) {
                                processConfig();
                            } else {
                                if (processTCPmessageCallback != nullptr) {
                                    processTCPmessageCallback(&data);
                                }
                            }
                            break;
                        default:
                            if (processTCPmessageCallback != nullptr) {
                                processTCPmessageCallback(&data);
                            }
                            break;
                        }
                    }
                }
            }
        } else {
            client.stop();
            client = wifiServer.accept();
            client.setTimeout(2000);
        }
    }
    
#ifdef ESP8266
    MDNS.update();
#endif
}

// ---------------------------------------- MQTT ---------------------------------------

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages) : mqttClient(MQTT_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;

#ifdef ESP32
    xTaskCreatePinnedToCore(this->checkConnectionTask, "Task1", 10000, this, 0, &mqttConnectTask, 0);
#endif
}

MessageData DashioMQTT::data(MQTT_CONN, INCOMING_BUFFER_SIZE);

void DashioMQTT::messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length) {
    data.processMessage(String(payload)); // The message components are stored within the connection where the messageReceived flag is set
}

void DashioMQTT::sendMessage(const String& message, MQTTTopicType topic) {
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
        if (length == (MQTT_BUFFER_SIZE / 2)) {
            sendMessage(message);
            message = "";
            length = 0;
        }
    }
    if (message.length() > 0) {
        sendMessage(message);
    }
    sendMessage(String('\n'));
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
}

void DashioMQTT::hostConnect() {
    Serial.print(F("Connecting to MQTT..."));
    state = connecting;
    if (mqttClient.connect(dashioDevice->deviceID.c_str(), username, password, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
        if (printMessages) {
            Serial.print(F("connected "));
            Serial.println(String(mqttClient.returnCode()));
        }
        state = serverConnected;
    } else {
        if (printMessages) {
            Serial.print(F("Failed - Try again in 10 seconds. E = "));
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
        if (mqttConn != NULL) {
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

            if (passThrough) {
                if (processMQTTmessageCallback != nullptr) {
                    processMQTTmessageCallback(&data);
                }
            } else {
                switch (data.control) {
                    case who:
                        sendMessage(dashioDevice->getWhoMessage());
                        break;
                    case connect:
                        sendMessage(dashioDevice->getConnectMessage());
                        break;
                    case config:
                        dashioDevice->dashboardID = data.idStr;
                        if (dashioDevice->configC64Str != NULL) {
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
        }

        data.checkBuffer();
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
int DashioBLE::connHandle;
bool DashioBLE::authenticated;
bool DashioBLE::authRequestConnect;
bool DashioBLE::printMessages;

class SecurityBLECallbacks : public BLESecurityCallbacks {
    bool onConfirmPIN(uint32_t pin) {
//        Serial.print(F("Confirm Pin: "));
//        Serial.println(pin);
        return true;  
    }
  
    uint32_t onPassKeyRequest() {
//        Serial.println(F("onPassKeyRequest"));
        return 0;
    }

    void onPassKeyNotify(uint32_t pass_key) {
//        Serial.println(F("onPassKeyNotify"));
    }

    bool onSecurityRequest() {
//        Serial.println(F("onSecurityRequest"));
        return true;
    }

    void onAuthenticationComplete(ble_gap_conn_desc * desc) {
        if (desc->sec_state.authenticated) {
            if (DashioBLE::printMessages) {
                Serial.println(F("BLE Authenticated"));
            }
            DashioBLE::authenticated = true;
            DashioBLE::authRequestConnect = true;
        } else {
            if (DashioBLE::printMessages) {
                Serial.println(F("BLE Authentication FAIL"));
            }
            DashioBLE::authenticated = false;
        }
        if (DashioBLE::printMessages) {
            if (desc->sec_state.encrypted) {
                Serial.println(F("BLE Encrypted"));
            }
            if (desc->sec_state.bonded) {
                Serial.println(F("BLE Bonded"));
            }
        }
    }
};

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        if (DashioBLE::printMessages) {
            Serial.println("BLE Server Connected");
        }
        DashioBLE::connHandle = desc->conn_handle;
    }
    
    void onDisconnect(BLEServer* pServer) {
        if (DashioBLE::printMessages) {
            Serial.println("BLE Server Disconnected");
        }
        DashioBLE::authenticated = false;
    }
};

// BLE callback for when a message is received
class MessageReceivedBLECallback: public BLECharacteristicCallbacks {
     public:
         MessageReceivedBLECallback(DashioBLE * local_DashioBLE):
            local_DashioBLE(local_DashioBLE) {}
        
         void onWrite(BLECharacteristic *pCharacteristic) {
             std::string payload = pCharacteristic->getValue();
             local_DashioBLE->data.processMessage(payload.c_str()); // The message components are stored within the connection where the messageReceived flag is set
         }
    
     public:
         DashioBLE * local_DashioBLE = NULL;
};

DashioBLE::DashioBLE(DashioDevice *_dashioDevice, bool _printMessages) : data(BLE_CONN, INCOMING_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

void DashioBLE::bleNotifyValue(const String& message) {
    pCharacteristic->setValue(message);
    pCharacteristic->notify();
}

void DashioBLE::sendMessage(const String& message) {
    if (pServer->getConnectedCount() > 0) {
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
    
void DashioBLE::run() {
    if (authRequestConnect) {
        authRequestConnect = false;
        sendMessage(dashioDevice->getConnectMessage());
    }
    
    if (data.messageReceived) {
        data.messageReceived = false;
        
        if (printMessages) {
            Serial.println(data.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(data.control)));
        }
        
        if (passThrough) {
            if (processBLEmessageCallback != nullptr) {
                processBLEmessageCallback(&data);
            }
        } else {
            switch (data.control) {
                case who:
                    sendMessage(dashioDevice->getWhoMessage());
                    break;
                case connect:
                    if (secureBLE && !authenticated) {
                        NimBLEDevice::startSecurity(connHandle);
                    } else {
                        sendMessage(dashioDevice->getConnectMessage());
                    }
                    break;
                case config:
                    dashioDevice->dashboardID = data.idStr;
                    if (dashioDevice->configC64Str != NULL) {
                        sendMessage(dashioDevice->getC64ConfigMessage());
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
    }
    
    data.checkBuffer();
}

void DashioBLE::end() {
    NimBLEDevice::deinit(true);
}

void DashioBLE::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processBLEmessageCallback = processIncomingMessage;
}
        
void DashioBLE::begin(int passKey) {
    authRequestConnect = false;
    secureBLE = (String(passKey).length() == 6);

    String localName = F("DashIO_");
    localName += dashioDevice->type;
    NimBLEDevice::init(localName.c_str());
    NimBLEDevice::setMTU(BLE_MAX_SEND_MESSAGE_LENGTH);
    
    // Setup BLE security (optional)
    if (secureBLE) {
        NimBLEDevice::setSecurityAuth(true, true, true); // bool bonding, bool mitm (man in the middle), bool sc
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // uint8_t iocap Sets the Input/Output capabilities of this device.
        NimBLEDevice::setSecurityPasskey(passKey);
        NimBLEDevice::setSecurityCallbacks(new SecurityBLECallbacks());
    } else {
        authenticated = true;
    }
    
    // Setup server, service and characteristic
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
    pCharacteristic->setCallbacks(new MessageReceivedBLECallback(this));
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

void DashioBLE::advertise(){
    pAdvertising->start();
}

bool DashioBLE::isConnected(){
    return (pServer->getConnectedCount() > 0);
}


// -------------------------------------------------------------------------------------

#endif
#endif
