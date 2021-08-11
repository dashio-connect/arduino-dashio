#include "DashioNano33.h"

// MQTT
const int  MQTT_QOS     = 1; // QoS = 1, as 2 causes subscribe to fail
const int  MQTT_RETRY_S = 10; // Retry after 10 seconds
const char *MQTT_SERVER = "dash.dashio.io";
const int  MQTT_PORT    = 8883;

// BLE
const int BLE_MAX_SEND_MESSAGE_LENGTH = 100;

// ---------------------------------------- WiFi ---------------------------------------

void DashioNano33_WiFi::connect(char *ssid, char *password) {
    // Check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("WiFi module failed!");
        while (true);
    }

    // Connect to Wifi Access Point
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
    
        // Connect to WPA/WPA2 network
        WiFi.disconnect();
        delay(1000);
        status = WiFi.begin(ssid, password);

        // wait 1 second for connection:
        delay(1000);
    }

    // Device IP address
    IPAddress ip = WiFi.localIP(); 
    Serial.print("IP Address: ");
    Serial.println(ip);
}

byte * DashioNano33_WiFi::macAddress() {
    byte mac[6];
    WiFi.macAddress(mac);
    return mac;
}

// ---------------------------------------- TCP ----------------------------------------

DashioNano33_TCP::DashioNano33_TCP(DashioDevice *_dashioDevice, uint16_t _tcpPort, bool _printMessages) : wifiServer(_tcpPort), dashioConnection(TCP_CONN) {
    dashioDevice = _dashioDevice;
    tcpPort = _tcpPort;
    printMessages = _printMessages;
}

void DashioNano33_TCP::setup(void (*processIncomingMessage)(DashioConnection *connection)) {
    processTCPmessageCallback = processIncomingMessage;
}

void DashioNano33_TCP::sendMessage(const String& message) {
    if (client.connected()) {
        client.print(message);

        if (printMessages) {
            Serial.println(F("---- TCP Sent ----"));
            Serial.println(message);
        }
    }
}

/*???
void DashioNano33_TCP::setupmDNSservice() {
    if (!client.begin("nano33iot")) {
       Serial.println(F("Error starting mDNS"));
       return;
    }
    Serial.println(F("mDNS started"));
    client.addService("DashIO", "tcp", tcpPort);
}

void DashioNano33_TCP::updatemDNS() {
    MDNS.update();
}
*/

void DashioNano33_TCP::startupServer() {
    wifiServer.begin();
}

void DashioNano33_TCP::checkForMessage() {
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

DashioNano_MQTT::DashioNano_MQTT(DashioDevice *_dashioDevice, int _bufferSize, bool _sendRebootAlarm, bool _printMessages) {
    dashioDevice = _dashioDevice;
    bufferSize = _bufferSize;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
    mqttClient = PubSubClient(wifiClient);
}

DashioConnection DashioNano_MQTT::dashioConnection(MQTT_CONN);

void DashioNano_MQTT::messageReceivedMQTTCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    dashioConnection.processMessage(message); // The message components are stored within the connection where the messageReceived flag is set
}

void DashioNano_MQTT::sendMessage(const String& message, MQTTTopicType topic) {
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

void DashioNano_MQTT::sendAlarmMessage(const String& message) {
    sendMessage(message, alarm_topic);
}

void DashioNano_MQTT::checkForMessage() {
    if (mqttClient.loop()) {
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

void DashioNano_MQTT::hostConnect() { // Non-blocking
    String willTopic = dashioDevice->getMQTTTopic(username, will_topic);
    Serial.print(F("LWT topic: "));  
    Serial.println(willTopic);

    Serial.print(F("Connecting to MQTT..."));
    String offlineMessage = dashioDevice->getOfflineMessage();
    if (mqttClient.connect(dashioDevice->deviceID.c_str(), username, password, willTopic.c_str(), MQTT_QOS, false, offlineMessage.c_str())) {
        Serial.print(F("connected"));

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

void DashioNano_MQTT::setup(char *_username, char *_password, void (*processIncomingMessage)(DashioConnection *connection)) {
    username = _username;
    password = _password;
    processMQTTmessageCallback = processIncomingMessage;

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(messageReceivedMQTTCallback);
    mqttClient.setBufferSize(bufferSize);
    mqttClient.setSocketTimeout(10);
    mqttClient.setKeepAlive(10);
}

void DashioNano_MQTT::checkConnection() {
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

DashioNano_BLE::DashioNano_BLE(DashioDevice *_dashioDevice, bool _printMessages) : bleService(SERVICE_UUID),
                                                                                   bleCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWriteWithoutResponse | BLENotify, BLE_MAX_SEND_MESSAGE_LENGTH, false) {
    dashioDevice = _dashioDevice;
    printMessages = _printMessages;
}

DashioConnection DashioNano_BLE::dashioConnection(BLE_CONN);

void DashioNano_BLE::onReadValueUpdate(BLEDevice central, BLECharacteristic characteristic) {
    // central wrote new value to characteristic
    int dataLength = characteristic.valueLength();
    char data[dataLength];
    int finalLength = characteristic.readValue(data, dataLength);
    for (int i = 0; i < dataLength; i++) {
        if (dashioConnection.processChar(data[i])) {
            dashioConnection.messageReceived = true;
        }
    }
}

void DashioNano_BLE::setup(void (*processIncomingMessage)(DashioConnection *connection)) {
    processBLEmessageCallback = processIncomingMessage;

    if (BLE.begin()) {
        // set advertised local name and service UUID:
        String localName = F("DashIO_");
        localName += dashioDevice->type;
        Serial.print("BLE local name: ");
        Serial.println(localName);
        BLE.setLocalName(localName.c_str());
        
        BLE.setConnectable(true);
        BLE.setAdvertisedService(bleService);

        // add the characteristic to the service
        bleService.addCharacteristic(bleCharacteristic);

        // add service
        BLE.addService(bleService);

        // Event driven reads.
        bleCharacteristic.setEventHandler(BLEWritten, onReadValueUpdate);

        // start advertising
        BLE.advertise();
    } else {
        Serial.println(F("Starting BLE failed"));
    }

}

void DashioNano_BLE::sendMessage(const String& message) {
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


void DashioNano_BLE::checkForMessage() {
    BLE.poll(); // Required for event handlers
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

// -------------------------------------------------------------------------------------
