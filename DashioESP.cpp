#if defined ESP32 || defined ESP8266

#include "DashioESP.h"

#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes

// MQTT
const int   MQTT_QOS     = 2;
const int   MQTT_RETRY_S = 10; // Retry after 10 seconds
const char *MQTT_SERVER  = "dash.dashio.io";
const int   MQTT_PORT    = 8883;

// ---------------------------------------- WiFi ---------------------------------------

void DashioESP_WiFi::setup(char *ssid, char *password, void (*connectCallback)(void)) {
    wifiConnectCallback = connectCallback;

    // Below is required to get WiFi to connect reliably on ESP32
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    WiFi.begin(ssid, password);
    wifiConnectCount = 1;
}

void DashioESP_WiFi::checkConnection() {
    // First, check WiFi and connect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount++;
        Serial.print(F("Connecting to Wi-Fi "));
        Serial.println(String(wifiConnectCount));

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
        }
    }
}

String DashioESP_WiFi::macAddress() {
    return WiFi.macAddress();
}

// ---------------------------------------- TCP ----------------------------------------

#ifdef ESP32
DashioESP_TCP::DashioESP_TCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages) : dashioConnection(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
    wifiServer = WiFiServer(_tcpPort);
}
#elif ESP8266
DashioESP_TCP::DashioESP_TCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages) : wifiServer(_tcpPort), dashioConnection(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
}
#endif

void DashioESP_TCP::setup(void (*processIncomingMessage)(DashioConnection *connection)) {
    processTCPmessageCallback = processIncomingMessage;
}

void DashioESP_TCP::sendMessage(const String& message) {
    if (client.connected()) {
        client.print(message);

        if (printMessages) {
            Serial.println(F("---- TCP Sent ----"));
            Serial.println(message);
        }
    }
}

void DashioESP_TCP::setupmDNSservice() {
#ifdef ESP32
    if (!MDNS.begin("esp32")) {
#elif ESP8266
    if (!MDNS.begin("esp8266")) {
#endif
       Serial.println(F("Error starting mDNS"));
       return;
    }
    Serial.println(F("mDNS started"));
    MDNS.addService("DashIO", "tcp", tcpPort);
}

#ifdef ESP8266
void DashioESP_TCP::updatemDNS() {
    MDNS.update();
}
#endif

void DashioESP_TCP::startupServer() {
    wifiServer.begin();
}

void DashioESP_TCP::checkForMessage() {
    if (!client) {
        client = wifiServer.available();
        client.setTimeout(2000);
    } else {
       if (client.connected()) {
            while (client.available()>0) {
                char c = client.read();
                if (dashioConnection.processChar(c)) {
                    if (printMessages) {
                        Serial.println(dashioConnection.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(dashioConnection.control)));
                    }
      
                    switch (dashioConnection.control) {
                    case who:
                        sendMessage(dashioDevice->getWhoMessage());
                        break;
                    case connect:
                        sendMessage(dashioDevice->getConnectMessage());
                        break;
                    default:
                        processTCPmessageCallback(&dashioConnection);
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

// ---------------------------------------- MQTT ---------------------------------------

DashioESP_MQTT::DashioESP_MQTT(DashioDevice *_dashioDevice, int bufferSize, bool _sendRebootAlarm, bool _printMessages) : mqttClient(bufferSize) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
}

DashioConnection DashioESP_MQTT::dashioConnection(MQTT_CONN);

void DashioESP_MQTT::messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length) {
    dashioConnection.processMessage(String(payload)); // The message components are stored within the connection where the messageReceived flag is set
}

void DashioESP_MQTT::sendMessage(const String& message, MQTTTopicType topic) {
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

void DashioESP_MQTT::sendAlarmMessage(const String& message) {
    sendMessage(message, alarm_topic);
}

void DashioESP_MQTT::checkForMessage() {
    if (mqttClient.connected()) {
        mqttClient.loop();
        if (dashioConnection.messageReceived) {
            dashioConnection.messageReceived = false;

            if (printMessages) {
                Serial.println(dashioConnection.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(dashioConnection.control)));
            }

            switch (dashioConnection.control) {
            case who:
                sendMessage(dashioDevice->getWhoMessage());
                break;
            case connect:
                sendMessage(dashioDevice->getConnectMessage());
                break;
            default:
                processMQTTmessageCallback(&dashioConnection);
                break;
            }
        }
    }
}

void DashioESP_MQTT::hostConnect() { // Non-blocking
    Serial.print(F("Connecting to MQTT..."));
#ifdef ESP8266
    wifiClient.setInsecure(); // For MQTT SSL
#endif

    if (mqttClient.connect(dashioDevice->deviceID.c_str(), username, password, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
        Serial.print(F("connected "));
        Serial.println(String(mqttClient.returnCode()));

        // Subscribe to private MQTT connection
        String subscriberTopic = dashioDevice->getMQTTSubscribeTopic(username);
        mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS); // ... and subscribe

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
        Serial.println(String(mqttClient.lastError()) + " ... " + mqttClient.returnCode());
    }
}

void DashioESP_MQTT::setupLWT() {
    // Setup MQTT Last Will and Testament message (Optional). Default keep alive time is 10s

    String willTopic = dashioDevice->getMQTTTopic(username, will_topic);
    Serial.print(F("LWT topic: "));  
    Serial.println(willTopic);

    String offlineMessage = dashioDevice->getOfflineMessage();
    mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);

    Serial.print(F("LWT message: "));
    Serial.println(offlineMessage);
}

void DashioESP_MQTT::setup(char *_username, char *_password, void (*processIncomingMessage)(DashioConnection *connection)) {
    username = _username;
    password = _password;
    processMQTTmessageCallback = processIncomingMessage;

    mqttClient.begin(MQTT_SERVER, MQTT_PORT, wifiClient);
    mqttClient.setOptions(10, true, 10000);  // 10s timeout
    mqttClient.onMessageAdvanced(messageReceivedMQTTCallback);
  
    setupLWT(); // Once the deviceID is known
}

void DashioESP_MQTT::checkConnection() {
    // Check and connect MQTT as necessary
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            if (mqttConnectCount == 0) {
                hostConnect();
            }
            if (mqttConnectCount >= MQTT_RETRY_S) {
                mqttConnectCount = 0;
            } else {
                mqttConnectCount++;
            }
        } else {
            mqttConnectCount = 0;
        }
    }
}

// ---------------------------------------- BLE ----------------------------------------
#ifdef ESP32
class securityBLECallbacks : public BLESecurityCallbacks {
    bool onConfirmPIN(uint32_t pin){
        Serial.print(F("Confirm Pin: "));
        Serial.println(pin);
        return true;  
    }
  
    uint32_t onPassKeyRequest(){
        Serial.println(F("onPassKeyRequest"));
        return 0;
    }

    void onPassKeyNotify(uint32_t pass_key){
        Serial.println(F("onPassKeyNotify"));
    }

    bool onSecurityRequest(){
        Serial.println(F("onSecurityRequest"));
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
        if(cmpl.success){
            Serial.println(F("Authentication Success"));
        } else{
            Serial.println(F("Authentication Fail"));
        }
    }
};

// BLE callback for when a message is received
class messageReceivedBLECallback: public BLECharacteristicCallbacks {

     public:
         messageReceivedBLECallback(DashioESP_BLE * local_DashioESP_BLE):
            local_DashioESP_BLE(local_DashioESP_BLE)
         {}
        
        void onWrite(BLECharacteristic *pCharacteristic) {
            std::string payload = pCharacteristic->getValue();
            local_DashioESP_BLE->dashioConnection.processMessage(payload.c_str()); // The message components are stored within the connection where the messageReceived flag is set
        }
    
    public:
        DashioESP_BLE * local_DashioESP_BLE = NULL;
};

DashioESP_BLE::DashioESP_BLE(DashioDevice *_dashioDevice, bool _printMessages) : dashioConnection(BLE_CONN) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

void DashioESP_BLE::bleNotifyValue(const String& message) {
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
}

void DashioESP_BLE::sendMessage(const String& message) {
    if (pServer->getConnectedCount() > 0) {
        int maxMessageLength = BLEDevice::getMTU() - 3;
        
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

void DashioESP_BLE::checkForMessage() {
     if (dashioConnection.messageReceived) {
        dashioConnection.messageReceived = false;

        if (printMessages) {
            Serial.println(dashioConnection.getReceivedMessageForPrint(dashioDevice->getControlTypeStr(dashioConnection.control)));
        }

        switch (dashioConnection.control) {
        case who:
            sendMessage(dashioDevice->getWhoMessage());
            break;
        case connect:
            sendMessage(dashioDevice->getConnectMessage());
            break;
        default:
            processBLEmessageCallback(&dashioConnection);
            break;
        }
    }
}

void DashioESP_BLE::setup(void (*processIncomingMessage)(DashioConnection *connection), bool secureBLE) {
    processBLEmessageCallback = processIncomingMessage;

    esp_bt_controller_enable(ESP_BT_MODE_BLE); // Make sure we're only using BLE
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT); // Release memory for Bluetooth Classic as we're not using it

    String localName = F("DashIO_");
    localName += dashioDevice->type;
    BLEDevice::init(localName.c_str());
    BLEDevice::setMTU(512);
    
    // Setup BLE security (optional)
    if (secureBLE) {
        BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
        BLEDevice::setSecurityCallbacks(new securityBLECallbacks());
        BLESecurity *pSecurity = new BLESecurity();
        pSecurity->setKeySize(16);
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
        pSecurity->setCapability(ESP_IO_CAP_NONE);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    }
    
    // Setup server, service and characteristic
    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY );
    pCharacteristic->setCallbacks(new messageReceivedBLECallback(this));
    pService->start();
    
    // Setup BLE advertising
    BLEAdvertising *pAdvertising;
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->start();
}

// -------------------------------------------------------------------------------------

#endif
#endif
