#include <WiFiClientSecure.h> // espressif library
#include <MQTT.h> // arduino-mqtt library created by Joël Gähwiler.
#include "DashIO.h"
#ifndef NOSAVE
    #include <EEPROM.h>
#endif
#include <ESPmDNS.h>

#ifndef DEVICE_TYPE
    #define DEVICE_TYPE "ESP32"
#endif

#ifndef DEVICE_NAME
    #define DEVICE_NAME "DashIO32"
#endif

#define EEPROM_SIZE 244

// WiFi
#define WIFI_TIMEOUT_S 300 // Restart after 5 minutes

// TCP
#ifndef TCP_PORT
    #define TCP_PORT 5000
#endif

// MQTT
#define MQTT_SERVER    "dash.dashio.io"
#define MQTT_PORT      8883
#define MQTT_QOS       2
#define MQTT_RETRY_S   10 // Retry after 10 seconds

// Create WiFI and MQTT
WiFiClientSecure wifiClient;
MQTTClient mqttClient(1024); // Buffer size = 1024
WiFiClient client; // For TCP
WiFiServer wifiServer(TCP_PORT); // For TCP

// Create device
DashDevice myDevice; //??? remame to dvce
  
// Create Connections
DashConnection tcpConnection(TCP_CONN);
DashConnection mqttConnection(MQTT_CONN); //??? rename to mqttConnection

void (*processMessageCallback)(DashConnection *connection);
void (*wifiConnectCallback)(void);

bool oneSecond = false; // Set by hardware timer every second.
int wifiConnectCount = 1;
int mqttConnectCount = 0;
String messageToSend = "";

struct ProvisioningObject {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[63];
    char dashUserName[32];
    char dashPassword[32];
    char saved;
};

ProvisioningObject userSetup;

// MQTT callback for when a message is received
static void messageReceivedMQTTCallback(MQTTClient *client, char *topic, char *payload, int payload_length) {
    mqttConnection.processMessage(String(payload)); // The message components are stored within the connection where the messageReceived flag is set
}

char *ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

void saveUserSetup() {
#ifndef NOSAVE
Serial.println("Saving to EEPROM");//???
Serial.println(userSetup.dashUserName);//???
    EEPROM.put(0, userSetup);
    EEPROM.commit();
#endif
}

void loadUserSetup() {
#ifndef NOSAVE
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println(F("Failed to init EEPROM"));
    } else {
        ProvisioningObject userSetupRead;
        EEPROM.get(0, userSetupRead);
        if (userSetupRead.saved != 'Y') {
            saveUserSetup();
        } else {
            userSetup = userSetupRead;
        }
    
    }
#endif

#ifdef PRNT
    Serial.print(F("Device Name: "));
    Serial.println(userSetup.deviceName);
    Serial.print(F("WiFi SSID: "));
    Serial.println(userSetup.wifiSSID);
    Serial.print(F("WiFi password: "));
    Serial.println(userSetup.wifiPassword);
    Serial.print(F("Dash userame: "));
    Serial.println(userSetup.dashUserName);
    Serial.print(F("Dash password: "));
    Serial.println(userSetup.dashPassword);
#endif
}

void mqttPublish(String message, MQTTTopicType topic = data_topic) {
    if (mqttClient.connected()) {
         String publishTopic = myDevice.getMQTTTopic(userSetup.dashUserName, topic);
        mqttClient.publish(publishTopic.c_str(), message.c_str(), false, MQTT_QOS);

#ifdef PRNT
        Serial.println(F("---- MQTT Sent ----"));
        Serial.print(F("Topic: "));
        Serial.println(publishTopic);
        Serial.print(F("Message: "));
        Serial.println(message);
        Serial.println();
    } else {
        Serial.println(F("Send Failed - MQTT not connected"));
#endif
    }
}

void tcpWriteStr(String message) {
  if (client.connected()) {
#ifdef PRNT
    Serial.println(F("**** TCP Sent ****"));
    Serial.println(message);
    Serial.println();
    client.print(message);
#endif
  }
}

void sendMessage(ConnectionType connectionType, String message) {
  if (connectionType == TCP_CONN) {
    tcpWriteStr(message);
  } else if (connectionType == BLE_CONN) {
//???    bleWriteStr(message);
  } else {
    mqttPublish(message);
  }
}


void sendAlarmMessage(String message) {
    mqttPublish(message, alarm_topic);
}

void setupMQTT_LWT() {
    // Setup MQTT Last Will and Testament message (Optional). Default keep alive time is 10s
    String willTopic = myDevice.getMQTTTopic(userSetup.dashUserName, will_topic);
#ifdef PRNT
    Serial.print(F("LWT topic: "));  
    Serial.println(willTopic);
#endif

    String offlineMessage = myDevice.getOfflineMessage();
    mqttClient.setWill(willTopic.c_str(), offlineMessage.c_str(), false, MQTT_QOS);

#ifdef PRNT
    Serial.print("LWT message: ");
    Serial.println(offlineMessage);
#endif
}

void setupWiFiMQTT() {
    // Below is required to get WiFi to connect reliably on ESP32
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    WiFi.begin(userSetup.wifiSSID, userSetup.wifiPassword);
    wifiServer.begin(); // For TCP
    wifiConnectCount = 1;

    // Setup MQTT host
    mqttClient.begin(MQTT_SERVER, MQTT_PORT, wifiClient);
    mqttClient.setOptions(10, true, 10000);  // 10s timeout
    mqttClient.onMessageAdvanced(messageReceivedMQTTCallback);
  
    setupMQTT_LWT(); // Once the deviceID is known
}

void mqttHostConnect() { // Non-blocking
  Serial.print(F("Connecting to MQTT..."));
  if (mqttClient.connect(myDevice.deviceID.c_str(), userSetup.dashUserName, userSetup.dashPassword, false)) { // skip = false is the default. Used in order to establish and verify TLS connections manually before giving control to the MQTT client
#ifdef PRNT
    Serial.print(F("connected "));
    Serial.println(String(mqttClient.returnCode()));
#endif

    // Subscribe to private MQTT connection
    String subscriberTopic = myDevice.getMQTTSubscribeTopic(userSetup.dashUserName);
    mqttClient.subscribe(subscriberTopic.c_str(), MQTT_QOS); // ... and subscribe

    // Send MQTT ONLINE and WHO messages to connection (Optional)
    // WHO is only required here if using the Dash server and it must be send to the ANNOUNCE topic
    mqttPublish(myDevice.getOnlineMessage());
    mqttPublish(myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE), announce_topic); // Update announce topic with new name
#ifdef PRNT
  } else {
    Serial.print(F("Failed - Try again in 10 seconds: "));
    Serial.println(String(mqttClient.lastError()));
#endif
  }
}

void checkConnectivity() {
    // First, check WiFi and connect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount++;
#ifdef PRNT
        Serial.print(F("Connecting to Wi-Fi "));
        Serial.println(String(wifiConnectCount));
#endif
        if (wifiConnectCount > WIFI_TIMEOUT_S) { // If too many fails, restart the ESP32. Sometimes ESP32's WiFi gets tied up in a knot.
            ESP.restart();
        }
    } else {
        // WiFi OK
        if (wifiConnectCount > 0) {
            wifiConnectCount = 0; // So that we only execute the following once after WiFI connection
#ifdef PRNT 
            Serial.print("Connected with IP: ");
            Serial.println(WiFi.localIP());
#endif
            if(!MDNS.begin("esp32")) {
               Serial.println("Error starting mDNS");
               return;
            }
            Serial.println("mDNS started");
            MDNS.addService("DashIO", "tcp", TCP_PORT);
    
            wifiConnectCallback();
        }
    
        // Check and connect MQTT as necessary
        if (!mqttClient.connected()) {      
            if (mqttConnectCount == 0) {
                mqttHostConnect();
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

void processIncomingMessage(DashConnection *connection) {
    switch (connection->control) {
    case who:
        sendMessage(connection->connectionType, myDevice.getWhoMessage(userSetup.deviceName, DEVICE_TYPE));
        break;
    case connect:
        sendMessage(connection->connectionType, myDevice.getConnectMessage());
        break;
    case deviceName:
        if (connection->idStr != "") {
            connection->idStr.toCharArray(userSetup.deviceName, connection->idStr.length() + 1);
            saveUserSetup();
#ifdef PRNT 
            Serial.print(F("Updated Device Name: "));
            Serial.println(userSetup.deviceName);
#endif
        }
        sendMessage(connection->connectionType, myDevice.getDeviceNameMessage(userSetup.deviceName));
        break;
    case wifiSetup:
        connection->payloadStr.toCharArray(userSetup.wifiSSID, connection->payloadStr.length() + 1);
        connection->payloadStr2.toCharArray(userSetup.wifiPassword, connection->payloadStr2.length() + 1);
        saveUserSetup();
#ifdef PRNT 
        Serial.print(F("Updated WIFI SSID: "));
        Serial.println(userSetup.wifiSSID);
        Serial.print(F("Updated WIFI Password: "));
        Serial.println(userSetup.wifiPassword);
#endif
        setupWiFiMQTT();
        break;
    case dashioSetup:
        connection->idStr.toCharArray(userSetup.dashUserName, connection->idStr.length() + 1);
        connection->payloadStr.toCharArray(userSetup.dashPassword, connection->payloadStr.length() + 1);
        saveUserSetup();
#ifdef PRNT 
        Serial.print(F("Updated Dash username: "));
        Serial.println(userSetup.dashUserName);
        Serial.print(F("Updated Dash Password: "));
        Serial.println(userSetup.dashPassword);
#endif
        setupWiFiMQTT();
        break;
     }
     processMessageCallback(connection);
}

void checkIncomingMessages() {
    // Never process messages from within an interrupt.
    // Instead, look to see if the messageReceived flag is set for each connection.

      // Look for TCP messages
    if (!client) {
        client = wifiServer.available();
        client.setTimeout(2000);
    } else {
        if (client.connected()) {
            while (client.available()>0) {
                char c = client.read();
                if (tcpConnection.processChar(c)) {
                    Serial.println("**** TCP Received ****");
                    Serial.print(tcpConnection.deviceID);
                    Serial.print("   " + myDevice.getControlTypeStr(tcpConnection.control));
                    Serial.println("   " + tcpConnection.idStr + "   " + tcpConnection.payloadStr + "   " + tcpConnection.payloadStr2);
      
                    processIncomingMessage(&tcpConnection);
                }
            }
        } else {
            client.stop();
            client = wifiServer.available();
            client.setTimeout(2000);
        }
    }

    // Look for MQTT Messages
    if (mqttClient.connected()) {
        mqttClient.loop();
        if (mqttConnection.messageReceived) {
            mqttConnection.messageReceived = false;
            processIncomingMessage(&mqttConnection);
         }
    }
}

void checkOutgoingMessages() {
    // Never send messages from within an interrupt.
    // Instead, look to see if there is a meassge ready to send in messageToSend.
    if (messageToSend.length() > 0) {
      tcpWriteStr(messageToSend);
      mqttPublish(messageToSend);
      messageToSend = "";
    }
}

void oneSecondTimerTask(void *parameters) {
    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        oneSecond = true;
    }
}

void checkMessagesTask(void *parameters) {
    for(;;) {
        if (oneSecond) { // Tasks to occur every second
            oneSecond = false;
            checkConnectivity(); // Only check once a second. Therefore counters are based on 1s intervals.
        }

        checkIncomingMessages();
        checkOutgoingMessages();
    }
}

void setupDevice(void (*processMessage)(DashConnection *connection)) {
    processMessageCallback = processMessage;
    
    myDevice.setDeviceID(WiFi.macAddress()); // Get unique deviceID
#ifdef PRNT
    Serial.print(F("Device ID: "));
    Serial.println(myDevice.deviceID);
#endif

    setupWiFiMQTT();

    // Setup 1 second timer task for checking connectivity
    xTaskCreate(
        oneSecondTimerTask,
        "One Second",
        1024, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );

    // Main task for checking comms
    xTaskCreate(
        checkMessagesTask,
        "Check Messages",
        8192, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );
}
