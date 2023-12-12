#if defined ARDUINO_SAMD_MKRNB1500

#include "dashiomkr1500.h"

#define MQTT_BUFFER_SIZE 2048
#define INCOMING_MQTT_BUFFER_SIZE 512

const int NB_TIMEOUT_MS    = 30000;
const int MQTT_QOS         = 2;
const int MQTT_RETRY_S     = 10; // Retry after 10 seconds
const int MQTT_RETRY_COUNT = 10; // Retry 10 times

// ---------------------------------------- LTE ----------------------------------------

bool DashioLTE::oneSecond = false;

DashioLTE::DashioLTE(bool _printMessages) {
    printMessages = _printMessages;
}

void DashioLTE::begin(const char* _pin) {
    begin(_pin, "", "", "");    
}

void DashioLTE::begin(const char* _pin, const char* _apn) {
    begin(_pin, _apn, "", "");
}
    
void DashioLTE::begin(const char* _pin, const char* _apn, const char* _username, const char* _password) {
    pin = _pin;
    apn = _apn;
    username = _username;
    password = _password;
    
    pinMode(SARA_RESETN, OUTPUT); // Allow hard reset

    nbAccess.setTimeout(NB_TIMEOUT_MS);
    scannerNetworks.begin();

    timer.every(1000, onTimerCallback); // 1000ms
}

void DashioLTE::attachConnection(DashioMQTT *_mqttConnection) {
    mqttConnection = _mqttConnection;
}

String DashioLTE::getDeviceID() {
    return "IMEI:" + modem.getIMEI();
}

int DashioLTE::getSignalStrength() {
    String signalStrength = scannerNetworks.getSignalStrength();
    return signalStrength.toInt();
}

bool DashioLTE::onTimerCallback(void *argument) {
    oneSecond = true;
    return true; // to repeat the timer action - false to stop
}

void DashioLTE::resetCellModem() {
    cellConnected = false;

    MODEM.send("AT+CFUN=15");
    MODEM.waitForResponse(5000);
    delay(5000);
    
    int waitCounter = 0;
    do {
        delay(1000);
        if (printMessages) {
            Serial.print(".");
        }

        waitCounter++;
        if (waitCounter > 20) {
//            NVIC_SystemReset(); // processor software reset is not good enough
            MODEM.hardReset();
            modemIsReset = true;
            if (printMessages) {
                Serial.println("Modem has been HARD reset");
            }
            return;
        }
        
        MODEM.noop();
    } while (MODEM.waitForResponse(1000) != 1);
    modemIsReset = true;
    if (printMessages) {
        Serial.println("Modem has been reset");
    }
}

bool DashioLTE::connectToCellNet() {
    if (printMessages) {
        Serial.println("Connecting to cellular network");
    }

    NB_NetworkStatus_t networkStatus = nbAccess.begin(pin, apn, username, password, true, true); // last two fields = restartModem, synchronous
    if (networkStatus == NB_READY) {
        cellConnected = true;

        if (printMessages) {
            Serial.print("Current carrier: ");
            Serial.println(scannerNetworks.getCurrentCarrier());
            
            Serial.print("Signal Strength: ");
            Serial.print(scannerNetworks.getSignalStrength());
            Serial.println(" [0-31]");
        }
        
        return true;
    } else {
        if (printMessages) {
            // enum NB_NetworkStatus_t { NB_ERROR, IDLE, CONNECTING, NB_READY, GPRS_READY, TRANSPARENT_CONNECTED, NB_OFF};
            Serial.print("Failed to connect - resetting modem: Status: ");
            Serial.println(networkStatus);
        }
        resetCellModem();
    }

    return false;
}

void DashioLTE::run() {
    timer.tick();

    if (mqttConnection != NULL) {
        mqttConnection->run();
    }

    if (oneSecond) {
        oneSecond = false;
        
        if (cellConnected) {
            if (nbAccess.isAccessAlive()) {
                if (mqttConnection != NULL) {
                    if (mqttConnection->checkConnection()) {
                    } else {
                        mqttRetry++;
                        if (mqttRetry > MQTT_RETRY_COUNT) {
                            mqttRetry = 0;
                            resetCellModem();
                        }
                    }
                }
            } else {
                if (printMessages) {
                    Serial.println("No cellular connection");
                }
                cellConnected = false;
            }
        } else {
            if (connectToCellNet()) {
                if (mqttConnection->dashioDevice->deviceID == "") {
                    mqttConnection->dashioDevice->setup(getDeviceID());
                }
                mqttConnection->begin();
            }
        }
    }
}

// ---------------------------------------- MQTT ---------------------------------------

DashioMQTT::DashioMQTT(DashioDevice *_dashioDevice, bool _sendRebootAlarm, bool _printMessages) : mqttClient(MQTT_BUFFER_SIZE) {
    dashioDevice = _dashioDevice;
    sendRebootAlarm  = _sendRebootAlarm;
    printMessages = _printMessages;
}

MessageData DashioMQTT::data(MQTT_CONN, INCOMING_MQTT_BUFFER_SIZE);

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
    if (printMessages) {
        Serial.print(F("LWT topic: "));
        Serial.println(willTopic);
    }

    String offlineMessage = dashioDevice->getOfflineMessage();
    mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);
    if (printMessages) {
        Serial.print(F("LWT message: "));
        Serial.println(offlineMessage);
    }
}

void DashioMQTT::setCallback(void (*processIncomingMessage)(MessageData *messageData)) {
    processMQTTmessageCallback = processIncomingMessage;
}
    
void DashioMQTT::setup(const char *_username, const char *_password) {
    username = _username;
    password = _password;
    state = notReady;
}

void DashioMQTT::begin() {
    mqttClient.begin(mqttHost, mqttPort, nbsslCLient);
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
    if (printMessages) {
        Serial.print(F("Connecting to MQTT..."));
    }
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

bool DashioMQTT::checkConnection() {
    // Check and connect MQTT as necessary
    if (state == disconnected) {
        if (mqttConnectCount == 0) {
            hostConnect();
        }
        if (mqttConnectCount >= MQTT_RETRY_S) {
            mqttConnectCount = 0;
            return false;
        } else {
            mqttConnectCount++;
            return true;
        }
    } else if (state == serverConnected) {
        mqttConnectCount = 0;
        return true;
    }
}
    
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

# endif
